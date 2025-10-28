#![allow(non_snake_case, non_camel_case_types)]
pub mod client;
pub mod repository;

use bson::{doc, Array, Document};
use log::info;
use serde::{Deserialize, Serialize};
use std::error::Error;
use std::sync::Once;

pub use client::cpp::{Logger, Configuration};
use client::cpp::{init_logger, init as init_service, execute as exec};

static INIT: Once = Once::new();

/// Initialise the mongo-service client for Rust.
///
/// This function performs one-time (process-wide) initialisation for the client by:
/// - configuring the logger via `init_logger`
/// - establishing the native mongo-service connection via `init_service`
///
/// The initialisation is guarded by a `std::sync::Once`, so subsequent calls are
/// no-ops. This makes `init` safe to call from multiple threads; only the first
/// successful invocation will perform the initialisation.
///
/// Parameters
/// - `logger`: Logger configuration, including path, file name, and level.
/// - `config`: Service connection settings (e.g., host and port).
///
/// Notes
/// - If you need to change logging or connection parameters, do so before the
///   first call to `init`. Later calls will be ignored due to the `Once` guard.
/// - This function logs its progress using the provided logger settings.
///
/// Example
/// ```rust
/// use mongo_service::{init, Logger, Configuration};
///
/// fn main() {
///     let mut logger = Logger::new("/tmp/", "mongo-service-rust");
///     // Optionally, configure logger.level here if needed.
///
///     let config = Configuration::new("rust", "localhost", 2000);
///     // Safe to call multiple times; only the first call initialises.
///     init(logger, config);
/// }
/// ```
pub fn init(logger: Logger, config: Configuration)
{
  INIT.call_once(|| {
    info!("Initialising logger at {}", logger.path);
    init_logger(logger);
    info!("Initialising mongo-service connection at {}:{}", config.host, config.port);
    init_service(config);
  });
}

/// Represents various database actions that can be performed. Each variant corresponds to a specific operation.
///
/// # Variants
///
/// - `create`: Create a new document in the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#create).
/// - `createTimeseries`: Create a new time-series record. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#create-timeseries).
/// - `retrieve`: Retrieve a document or documents from the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#retrieve).
/// - `update`: Update an existing document in the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#update).
/// - `delete`: Delete a document or documents from the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#delete).
/// - `count`: Count the number of documents matching a query in the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#count).
/// - `distinct`: Retrieve distinct values for a specified field in the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#distinct).
/// - `createCollection`: Create a new collection in the database. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#create-collection).
/// - `renameCollection`: Rename an existing collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#rename-collection).
/// - `dropCollection`: Drop a collection, removing it from the database. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#drop-collection).
/// - `index`: Create an index on a collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#index).
/// - `dropIndex`: Drop an index from a collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#drop-index).
/// - `bulk`: Perform bulk write operations on the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#bulk-write).
/// - `pipeline`: Execute an aggregation pipeline on the collection. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#aggregation-pipeline).
/// - `transaction`: Perform a transaction involving multiple operations. [More details](https://github.com/sptrakesh/mongo-service?tab=readme-ov-file#transaction).
#[derive(Clone, Debug, Deserialize, Serialize)]
pub enum Action
{
  /// Create a new document in the collection.
  create,
  /// Create a new time-series record.
  createTimeseries,
  /// Retrieve a document or documents from the collection.
  retrieve,
  /// Update existing document(s) in the collection.
  update,
  /// Delete a document or documents from the collection.
  delete,
  /// Count the number of documents matching a query in the collection.
  count,
  /// Retrieve distinct values for a specified field in the collection.
  distinct,
  /// Create a new collection in the database.
  createCollection,
  /// Rename an existing collection.
  renameCollection,
  /// Drop a collection, removing it from the database.
  dropCollection,
  /// Ensure an index on a collection.
  index,
  /// Drop an index from a collection.
  dropIndex,
  /// Perform bulk write operations on the collection.
  bulk,
  /// Execute an aggregation pipeline on the collection.
  pipeline,
  /// Perform a transaction involving multiple operations.
  transaction
}

/// Represents a database operation request, encapsulating details such as the targeted database,
/// collection, document, options, and other metadata required for performing the operation.
///
/// # Fields
///
/// - `database` (`String`):
///   The name of the database where the operation will be executed.
///
/// - `collection` (`String`):
///   The name of the collection within the database on which the operation will be performed.
///
/// - `document` (`Document`):
///   The main document associated with the request. This is typically the primary data being inserted,
///   updated, or queried.
///
/// - `options` (`Option<Document>`):
///   Optional settings or configurations that may influence how the request is processed.
///   For example, query options, execution preferences, or custom parameters.
///
/// - `metadata` (`Option<Document>`):
///   Optional metadata to provide contextual information relevant to the request.
///   This can include tracking-related data or additional descriptive information.
///
/// - `correlationId` (`Option<String>`):
///   An optional unique identifier used to correlate this request with other related operations or logs.
///   Useful for tracing and debugging purposes.
///
/// - `action` (`Action`):
///   Specifies the type of action to be performed with this request, represented using an `Action` enum.
///
/// - `skipVersion` (`bool`):
///   A boolean flag that indicates whether or not versioning should be skipped for this operation.
///   Typically used in scenarios where version tracking is unnecessary or explicitly undesired.
///
/// - `skipMetric` (`bool`):
///   A boolean flag that indicates whether or not metrics associated with this request should be recorded.
///   Useful for excluding specific operations from system telemetry or analytics.
///
/// # Notes
///
/// This `Request` struct is primarily used to define all the necessary details for
/// database-related operations within the system. It provides a flexible mechanism for
/// handling various types of operations, including data mutations, queries, and metadata-related tasks.
///
#[derive(Debug)]
pub struct Request
{
  /// The database against which the operation will be executed.
  pub database: String,
  /// The collection against which the operation will be executed.
  pub collection: String,
  /// The BSON document associated with the request.
  pub document: Document,
  /// The MongoDB options associated with the request.
  pub options: Option<Document>,
  /// Custom metadata to add to the version history document created.  Only applies to `create`, `update`, and `delete` actions.
  pub metadata: Option<Document>,
  /// Optional *correlation id* to associate with the metric record created by this action.
  pub correlationId: Option<String>,
  /// The type of database action being performed.
  pub action: Action,
  /// Indicate whether to create a *version history*
  /// document for this operation.  Useful when creating non-critical data such as
  /// logs.
  pub skipVersion: bool,
  /// Indicate whether to create or skip creating a *metric* document for this operation.
  pub skipMetric: bool
}

impl Request
{
  /// Constructs a new `Request` instance with the specified database name, collection name, and document.
  ///
  /// This function initialises a `Request` with the mandatory parameters - `database`, `collection`, and `document`.
  /// Other optional parameters such as `options`, `metadata`, `correlationId`, `skipVersion`, and `skipMetric`
  /// are set to default values (`None` or `false`). The `action` is explicitly initialised as `Action::create`.
  ///
  /// # Arguments
  ///
  /// * `database` - A `String` representing the name of the database.
  /// * `collection` - A `String` representing the name of the collection inside the database.
  /// * `document` - A `Document` object that represents the content to be created within the database collection.
  ///   The `_id` field is *required*.
  ///
  /// # Returns
  ///
  /// * Returns an instance of `Request` initialised for the `create` action with provided parameters.
  ///
  /// # Example
  ///
  /// ```rust
  /// use mongo_service::Request;
  /// use bson::{doc, oid::ObjectId};
  /// let database = String::from("my_database");
  /// let collection = String::from("my_collection");
  /// let document = doc! {"_id": ObjectId::new()}; // Assume Document is properly initialised
  ///
  /// let request = Request::create(database, collection, document);
  /// ```
  ///
  /// # Notes
  ///
  /// This constructor is specifically intended for actions related to creating a document.
  /// If you are looking for other operations like update or delete, use other appropriate constructors or methods.
  pub fn create(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::create, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` for creating a time series document in a specified database and collection.
  ///
  /// # Arguments
  ///
  /// * `database` - A `String` that represents the name of the database where the time series will be created.
  /// * `collection` - A `String` that specifies the collection within the database for the time series.
  /// * `document` - A `Document` containing the time series data.  The `_id` field is optional.
  ///
  /// # Returns
  ///
  /// Returns an instance of `Request` with the relevant fields initialised for creating a time series document.
  ///
  /// # Example
  ///
  /// ```
  /// use bson::{doc, datetime::DateTime};
  /// use chrono::Utc;
  /// use mongo_service::Request;
  /// let database_name = String::from("my_database");
  /// let collection_name = String::from("my_collection");
  ///
  /// let request = Request::create_time_series(database_name, collection_name, doc!{"timestamp": DateTime::from_chrono(Utc::now())});
  /// ```
  ///
  /// The returned `Request` will have:
  /// - `options`, `metadata`, and `correlationId` set to `None`.
  /// - Fields `action` initialised to `Action::createTimeseries`.
  /// - `skipVersion` and `skipMetric` defaults set to `false`.
  ///
  /// This method is typically used to initialise requests for time series operations in data storage or management systems.
  pub fn create_time_series(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::createTimeseries, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` instance configured for retrieving documents from a database collection.
  ///
  /// # Parameters
  /// - `database`: A `String` specifying the name of the database.
  /// - `collection`: A `String` specifying the name of the collection within the database.
  /// - `document`: A `Document` that defines the query or filter criteria for the retrieval operation.
  ///
  /// # Returns
  /// A newly initialised `Request` instance containing the specified parameters with additional options
  /// configured for the retrieval operation:
  /// - A limit of 250 documents per query result set.
  /// - A default sorting order where documents are sorted in ascending order by the "_id" field.
  ///
  /// The constructed request also includes:
  /// - `options`: A `Some` value containing document-specific options (`limit` and `sort`).
  /// - `metadata`: Set to `None`.
  /// - `correlationId`: Set to `None`.
  /// - `action`: Set to `Action::retrieve` to indicate the retrieval operation.
  /// - `skipVersion`: Set to `false`, meaning the version will not be skipped.
  /// - `skipMetric`: Set to `false`, meaning metrics will not be skipped.
  ///
  /// # Example
  /// ```rust
  /// use mongo_service::Request;
  /// use bson::doc;
  ///
  /// let database = "my_database".to_string();
  /// let collection = "my_collection".to_string();
  /// let filter = doc! { "status": "active" };
  ///
  /// let request = Request::retrieve(database, collection, filter);
  /// ```
  pub fn retrieve(database: String, collection: String, document: Document) -> Self
  {
    let opts = doc!{"limit": 250, "sort": doc!{"_id": 1}};
    Request{database, collection, document, options: Some(opts), metadata: None, correlationId: None,
      action: Action::retrieve, skipVersion: false, skipMetric: false}
  }

  /// Creates a new `Request` instance configured for the `update` action.
  ///
  /// This function initialises a `Request` object with the provided database name, collection name, and document to update.
  /// It is commonly used to prepare a request to update an existing document in a database.
  ///
  /// # Parameters
  /// - `database` (String): The name of the database where the document exists.
  /// - `collection` (String): The name of the collection that contains the document to be updated.
  /// - `document` (Document): The document that holds the updated values.
  ///
  /// # Returns
  /// A `Request` instance with the specified database, collection, and document.
  /// The returned request is configured for an update action with no additional options, metadata, or correlation ID.
  ///
  /// # Default Fields
  /// - `options`: Set to `None` by default.
  /// - `metadata`: Set to `None` by default.
  /// - `correlationId`: Set to `None` by default.
  /// - `action`: Set to `Action::update` to specify the update operation.
  /// - `skipVersion`: Set to `false` by default.
  /// - `skipMetric`: Set to `false` by default.
  ///
  /// # Example
  /// ```
  /// use bson::{doc, oid::ObjectId};
  /// use mongo_service::Request;
  /// let database = String::from("example_db");
  /// let collection = String::from("users");
  /// let document = doc! {"_id": ObjectId::new(), "field": "value"}; // Assuming Document is properly defined and initialised
  ///
  /// let request = Request::update(database, collection, document);
  /// // Now, `request` is prepared for an update operation.
  /// ```
  pub fn update(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::update, skipVersion: false, skipMetric: false}
  }

  /// Creates a `Request` to delete a specified document from the given database and collection.
  ///
  /// This function initialises a `Request` object with the `delete` action to remove the provided `document`
  /// from the specified `database` and `collection`. The remaining fields (`options`, `metadata`, `correlationId`,
  /// `skipVersion`, `skipMetric`) are set to default values.
  ///
  /// # Parameters
  ///
  /// * `database` - A `String` representing the name of the database from which the document will be deleted.
  /// * `collection` - A `String` representing the name of the collection from which the document will be deleted.
  /// * `document` - The `Document` to be deleted from the specified database and collection.
  ///
  /// # Returns
  ///
  /// A `Request` configured to perform a delete operation.
  ///
  /// # Example
  ///
  /// ```rust
  /// use bson::{doc, oid::ObjectId};
  /// use mongo_service::Request;
  /// let request = Request::delete(
  ///     String::from("my_database"),
  ///     String::from("my_collection"),
  ///     doc! {"_id": ObjectId::new()}
  /// );
  /// ```
  pub fn delete(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::delete, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` object for counting documents in a database collection.
  ///
  /// # Parameters
  /// - `database`: A `String` representing the name of the database.
  /// - `collection`: A `String` specifying the name of the collection within the database.
  /// - `document`: A `Document` used as a filter to specify the criteria for counting documents.
  ///
  /// # Returns
  /// A new `Request` instance configured with the specified `database`, `collection`, and `document`,
  /// with the following default settings:
  /// - `options`: `None`
  /// - `metadata`: `None`
  /// - `correlationId`: `None`
  /// - `action`: `Action::count`
  /// - `skipVersion`: `false`
  /// - `skipMetric`: `false`
  ///
  /// # Example
  /// ```rust
  /// use bson::doc;
  /// use mongo_service::Request;
  /// let request = Request::count(
  ///     String::from("my_database"),
  ///     String::from("my_collection"),
  ///     doc!{},
  /// );
  /// ```
  ///
  /// This function is commonly used when you need to perform a document count action
  /// within a specific collection in a database.
  pub fn count(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::count, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` object to execute a `distinct` action on a specified database and collection.
  ///
  /// # Parameters
  ///
  /// - `database`: A `String` representing the name of the database to operate on.
  /// - `collection`: A `String` representing the name of the collection within the database.
  /// - `document`: A `Document` containing the criteria or query for the `distinct` action.
  ///
  /// # Returns
  ///
  /// Returns a `Request` object with the specified database, collection, and document, preconfigured for the `distinct` action.
  ///
  /// # Defaults
  ///
  /// - `options`: Defaults to `None`.
  /// - `metadata`: Defaults to `None`.
  /// - `correlationId`: Defaults to `None`.
  /// - `action`: Set to `Action::distinct` for this operation.
  /// - `skipVersion`: Defaults to `false`.
  /// - `skipMetric`: Defaults to `false`.
  ///
  /// # Example
  ///
  /// ```rust
  /// use bson::doc;
  /// use mongo_service::Request;
  /// let database = "my_database".to_string();
  /// let collection = "my_collection".to_string();
  /// let document = doc! {"field": "key"}; // Assume `Document` is a valid type.
  /// let request = Request::distinct(database, collection, document);
  /// ```
  pub fn distinct(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::distinct, skipVersion: false, skipMetric: false}
  }

  /// Creates a new instance of a `Request` to create a collection in the specified database.
  ///
  /// # Arguments
  ///
  /// * `database` - A `String` representing the name of the database where the collection
  ///                will be created.
  /// * `collection` - A `String` representing the name of the collection to be created.
  /// * `document` - A `Document` containing additional configuration for the collection.
  ///
  /// # Returns
  ///
  /// An instance of `Request` configured with the provided database, collection, and document,
  /// along with default settings for `options`, `metadata`, `correlationId`, and other flags.
  ///
  /// # Example
  ///
  /// ```rust
  /// use bson::doc;
  /// use mongo_service::Request;
  /// let database_name = String::from("my_database");
  /// let collection_name = String::from("my_collection");
  /// let document = doc! {};
  ///
  /// let request = Request::create_collection(database_name, collection_name, document);
  /// ```
  ///
  /// The returned `Request` is pre-configured with the `Action::createCollection` action.
  pub fn create_collection(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::createCollection, skipVersion: false, skipMetric: false}
  }

  /// Creates a `Request` instance for the action of renaming a collection in a database.
  ///
  /// # Parameters
  /// - `database` (String): The name of the database containing the collection to rename.
  /// - `collection` (String): The name of the collection to be renamed.
  /// - `document` (Document): A document containing the required `target` field with the new name.
  ///
  /// # Returns
  /// - `Self`: A `Request` object configured with the provided parameters for the `renameCollection` action.
  ///
  /// # Behaviour
  /// This function initialises a `Request` with the following:
  /// - The provided `database`, `collection`, and `document`.
  /// - The `action` attribute set to `Action::renameCollection`.
  /// - Optional `options`, `metadata`, and `correlationId` fields initialised as `None`.
  /// - `skipVersion` and `skipMetric` flags set to `false`.
  ///
  /// This function is typically used to generate a request to rename a collection in a specified database.
  pub fn rename_collection(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::renameCollection, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` instance to drop a specified collection from a database.
  ///
  /// # Arguments
  ///
  /// * `database` - A `String` representing the name of the database containing the collection to be dropped.
  /// * `collection` - A `String` specifying the name of the collection to be removed.
  /// * `document` - A `Document` object that can optionally hold a `clearVersionHistory` boolean
  ///   that indicates if all version history documents for the collection should be removed asynchronously.
  ///
  /// # Returns
  ///
  /// A new instance of the `Request` struct configured to execute the `dropCollection` action.
  ///
  /// # Notes
  ///
  /// - The `options`, `metadata`, and `correlationId` fields in the `Request` are set to `None` by default.
  /// - The `action` field is set to `Action::dropCollection`.
  /// - Both `skipVersion` and `skipMetric` flags are set to `false` by default.
  ///
  /// # Example
  ///
  /// ```rust
  /// use bson::doc;
  /// use mongo_service::Request;
  /// let database_name = String::from("example_db");
  /// let collection_name = String::from("example_collection");
  /// let doc = doc!{"clearVersionHistory": false};
  ///
  /// let request = Request::drop_collection(database_name, collection_name, doc);
  /// // Use the `request` as needed, e.g., passing it to a database handler.
  /// ```
  pub fn drop_collection(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::dropCollection, skipVersion: false, skipMetric: false}
  }

  /// Creates a new `Request` instance for the `index` action.
  ///
  /// This function is used to construct a `Request` object to ensure a corresponding index
  /// exists in the specified database and collection.
  ///
  /// # Parameters
  /// - `database`: A `String` representing the name of the database where the document will be indexed.
  /// - `collection`: A `String` representing the name of the collection within the database where the document will be stored.
  /// - `document`: A `Document` object that contains the index specification.
  ///
  /// # Returns
  /// - A new `Request` instance with the specified database, collection, and document.
  ///   Other fields, such as `options`, `metadata`, and `correlationId`, are initialised to `None`.
  ///   The `action` field is set to `Action::index`, and both `skipVersion` and `skipMetric` are set to `false`.
  ///
  /// # Example
  /// ```
  /// use bson::doc;
  /// use mongo_service::Request;
  ///
  /// let database = "my_database".to_string();
  /// let collection = "my_collection".to_string();
  /// let document = doc! {"key": 1};
  /// let request = Request::index(database, collection, document);
  /// ```
  pub fn index(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::index, skipVersion: false, skipMetric: false}
  }

  /// Constructs a new `Request` instance configured to drop an index from a specified collection in a database.
  ///
  /// # Parameters
  /// - `database` (`String`): The name of the target database where the operation will be performed.
  /// - `collection` (`String`): The name of the collection from which the index will be dropped.
  /// - `document` (`Document`): The document specifying details about the index to be dropped, such as index keys or name.
  ///
  /// # Returns
  /// - `Self`: A `Request` object preconfigured with the provided `database`, `collection`, and `document` values,
  ///   and with default values for optional properties (`options`, `metadata`, `correlationId`, `skipVersion`, and `skipMetric`).
  ///
  /// # Behaviour
  /// - The request is set to perform the `Action::dropIndex` operation.
  /// - Both `skipVersion` and `skipMetric` are set to `false` by default.
  ///
  /// # Example
  /// ```
  /// use bson::doc;
  /// use mongo_service::Request;
  /// let database = String::from("my_database");
  /// let collection = String::from("my_collection");
  /// let index_document = doc! {"key": 1};
  ///
  /// let request = Request::drop_index(database, collection, index_document);
  /// ```
  pub fn drop_index(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::dropIndex, skipVersion: false, skipMetric: false}
  }

  /// Creates a new `Request` instance for performing a bulk operation on a specified database and collection.
  ///
  /// # Arguments
  ///
  /// * `database` - A `String` representing the name of the database where the bulk operation will be performed.
  /// * `collection` - A `String` representing the name of the collection within the database.
  /// * `document` - A `Document` containing the data to be included in the bulk operation.
  ///
  /// # Returns
  ///
  /// A `Request` instance configured for a bulk action with the given `database`, `collection`, and `document`.
  ///
  /// # Behaviour
  ///
  /// - Sets the `action` field of the `Request` to `Action::bulk`.
  /// - Leaves the optional fields `options`, `metadata`, and `correlationId` as `None`.
  /// - Sets `skipVersion` and `skipMetric` to `false` by default.
  ///
  /// # Examples
  ///
  /// ```rust
  /// use bson::{bson, doc, oid::ObjectId};
  /// use mongo_service::Request;
  /// let request = Request::bulk(
  ///     "my_database".to_string(),
  ///     "my_collection".to_string(),
  ///     doc! {"insert": bson!([doc!{"_id": ObjectId::new()}, doc!{"_id": ObjectId::new()}])}
  /// );
  /// ```
  pub fn bulk(database: String, collection: String, document: Document) -> Self
  {
    Request{database, collection, document, options: None, metadata: None, correlationId: None,
      action: Action::bulk, skipVersion: false, skipMetric: false}
  }

  /// Creates a new `Request` object with the specified database name, collection name, and document
  /// which holds the aggregation pipeline `specification` array.
  ///
  /// The function initialises the `Request` with the given parameters and sets default values for
  /// unspecified fields such as `options`, `metadata`, `correlationId`, `skipVersion`, and `skipMetric`.
  /// The action is set to `Action::pipeline`.
  ///
  /// # Parameters:
  /// - `database`: A `String` representing the name of the database.
  /// - `collection`: A `String` representing the name of the collection.
  /// - `array`: An `Array` with the pipeline stages.  The constructor wraps these into the
  ///   appropriate `Document` structure.
  ///
  /// # Returns:
  /// - A newly created `Request` object initialised with the specified parameters and default values.
  ///
  /// # Example:
  /// ```rust
  /// use bson::{bson, doc};
  /// use mongo_service::Request;
  /// let db = String::from("example_database");
  /// let coll = String::from("example_collection");
  /// let specification = bson!([doc!{"$match": doc!{}},
  ///   doc!{"$sort": doc!{"_id": -1}},
  ///   doc!{"$limit": 100},
  ///   doc!{"$lookup": doc!{}}]);
  /// let specification = specification.as_array().unwrap();
  ///
  /// let request = Request::pipeline(db, coll, specification);
  /// ```
  pub fn pipeline(database: String, collection: String, specification: &Array) -> Self
  {
    Request{database, collection, document: doc!{"specification": specification}, options: None, metadata: None, correlationId: None,
      action: Action::pipeline, skipVersion: false, skipMetric: false}
  }

  /// Creates a new `Request` instance for executing a transaction.
  ///
  /// # Parameters
  /// - `database`: A `String` representing the name of the database where the transaction will be executed.
  /// - `collection`: A `String` specifying the name of the collection involved in the transaction.
  /// - `array`: An `Array` representing the operations to be executed within the transaction.  The
  ///   constructor wraps these into the appropriate `Document` structure.
  ///
  /// # Returns
  /// - `Self`: A new `Request` instance configured for the transaction operation.
  ///
  /// # Behaviour
  /// - The `Request` is initialised with the given `database`, `collection`, and `document`.
  /// - The `action` field is set to `Action::transaction` to indicate that this is a transaction request.
  /// - Optional fields like `options`, `metadata`, and `correlationId` are set to `None` by default.
  /// - The `skipVersion` and `skipMetric` flags are set to `false` by default.
  pub fn transaction(database: String, collection: String, items: &Array) -> Self
  {
    Request{database, collection, document: doc!{"items": items}, options: None, metadata: None,
      correlationId: None, action: Action::transaction, skipVersion: false, skipMetric: false}
  }
}

/// Documentation for the `Response` enum.
///
/// The `Response` enum represents the possible outcomes of a given operation,
/// providing flexibility to handle different kinds of results.
///
/// # Variants
///
/// - `result(Document)`:
///     Represents a successful operation that returns a single `Document`.
///     Typically used when the operation is expected to yield one specific result.
///     In contrast to the C++ API, this variant is also used to return the results
///     of operations such as `create`, `delete`, etc.
///
/// - `results(Array)`:
///     Represents the results of executing a query that returns multiple `Document`s.
///
/// - `error(String)`:
///     Represents an operation that encountered an error.
///     Carries a `String` describing the error message or details.
#[derive(Debug)]
pub enum Response
{
  /// Represents a successful operation that returns a single `Document`.
  result(Document),
  /// Represents the results of executing a query that returns multiple `Document`s.
  results(Array),
  /// Represents an operation that encountered an error.
  error(String)
}

/// Executes a given request and processes the response. The request typically involves database and collection operations,
/// paired with specific actions and optional metadata.
///
/// # Parameters
/// - `request`: A `Request` object containing details about the database, collection, document, action, and other optional properties.
///
/// # Returns
/// - `Result<Response, Box<dyn Error>>`:
///   - A successful execution will return a `Response` object.
///   - If an error occurs during execution, it will return an `Error` boxed as `Box<dyn Error>`.
///
/// # Behaviour
/// 1. Prepares a BSON document (`data`) with the primary request components (`database`, `collection`, `document`, `action`, etc.).
/// 2. Adds optional values to the BSON document if they are present in the `request`:
///    - `options`
///    - `metadata`
///    - `correlationId`
/// 3. Converts the prepared BSON data into a vector of bytes and sends it for execution using the `exec` function.
/// 4. Handles the response:
///    - If an error is returned by `exec`, it propagates the error.
///    - If the response contains a "result" key, wraps it as `Response::result`.
///    - If the response contains "results", wraps them as `Response::results`.
///    - If an "error" key exists, it wraps the error string as `Response::error`.
///    - If none of the above keys are present, returns the raw document as a `Response::result`.
///
/// # Notes
/// - The function assumes that the `response` returned by `exec` can be deserialized into a BSON `Document`.
/// - If a required deserialization step fails, the function will terminate with an error.
///
/// # Errors
/// - Returns an error if:
///   - The BSON document conversion (`to_vec`) fails.
///   - The `exec` function returns an error.
///   - The response cannot be deserialized into a `Document`.
///
/// # Example Usage
/// ```no_run
/// use bson::doc;
/// use mongo_service::{execute, Action, Request};
/// let request = Request {
///     database: "test_db".to_string(),
///     collection: "users".to_string(),
///     document: doc! { "name": "John Doe" },
///     action: Action::create,
///     skipVersion: false,
///     skipMetric: false,
///     options: None,
///     metadata: Some(doc! { "key": "value" }),
///     correlationId: Some("123".to_string()),
/// };
///
/// match execute(request) {
///     Ok(response) => println!("Response received: {:?}", response),
///     Ok(responses) => println!("Multiple documents received: {:?}", responses),
///     Err(e) => eprintln!("An error occurred: {}", e),
/// }
/// ```
pub fn execute(request: Request) -> Result<Response, Box<dyn Error>>
{
  let mut data = doc! {
    "database": request.database,
    "collection": request.collection,
    "document": request.document,
    "action": format!("{:?}", request.action),
    "skipVersion": request.skipVersion,
    "skipMetric": request.skipMetric
  };

  if request.options.is_some() { data.insert("options", request.options.unwrap()); }
  if request.metadata.is_some() { data.insert("metadata", request.metadata.unwrap()); }
  if request.correlationId.is_some() { data.insert("correlationId", request.correlationId.unwrap()); }

  let bytes = data.to_vec()?;
  let response = exec(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("result") { Ok(Response::result(d.get("result").unwrap().as_document().unwrap().clone())) }
  else if d.contains_key("results") { Ok(Response::results(d.get("results").unwrap().as_array().unwrap().clone())) }
  else if d.contains_key("error") { Ok(Response::error(d.get("error").unwrap().as_str().unwrap().to_string())) }
  else { Ok(Response::result(d)) }
}