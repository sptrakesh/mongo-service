use bson::{doc, serialize_to_document, Bson, Document};
use serde::{Deserialize, Serialize};

/// Options for retrieve operations.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Options
{
  /// The collation to use for text data.
  pub collation: Option<super::options::Collation>,
  /// Specifies the read preference level for the query.
  pub readPreference: Option<super::options::ReadPreference>,
  /// Specifies the fields to return in the documents that match the query filter.
  pub projection: Option<Document>,
  ///  The order of the documents returned in the result set. Fields specified in the sort, must have an index.
  pub sort: Option<Document>,
  pub commentOption: Option<Document>,
  /// The index to use. Specify either the index name as a string or the index specification document.
  pub hint: Option<Bson>,
  /// The exclusive upper bound for a specific index.
  pub max: Option<Document>,
  /// The inclusive lower bound for a specific index.
  pub min: Option<Document>,
  /// Adds a $comment to the query that shows in the profiler logs.
  pub comment: Option<String>,
  /// Specifies the time limit in milliseconds for the count operation to run before timing out.
  pub maxTime: Option<u64>,
  /// The maximum number of matching documents to return
  pub limit: Option<u64>,
  /// The number of matching documents to skip before returning results.
  pub skip: Option<u64>,
  /// Whether or not pipelines that require more than 100 megabytes of memory to execute write to temporary files on disk.
  pub allowDiskUse: Option<bool>,
  /// For queries against a sharded collection, allows the command (or subsequent getMore commands) to return partial results, rather than an error, if one or more queried shards are unavailable.
  pub allowPartialResults: Option<bool>,
  /// Whether only the index keys are returned for a query.
  pub returnKey: Option<bool>,
  /// If the $recordId field is added to the returned documents. The $recordId indicates the position of the document in the result set.
  pub showRecordId: Option<bool>
}

impl Options
{
  pub fn new() -> Self
  {
    Options {collation: None, readPreference: None, projection: None, sort: Some(doc!{ "_id": 1}),
      commentOption: None, hint: None, max: None, min: None, comment: None, maxTime: None,
      limit: Some(250), skip: None, allowDiskUse: None, allowPartialResults: None, returnKey: None,
      showRecordId: None}
  }
}

/// Request for retrieving data from the database.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Request<E>
{
  /// The application/client name.  This is added to the service metrics record.
  pub application: String,
  /// The database against which the operation will be executed.
  pub database: String,
  /// The collection against which the operation will be executed.
  pub collection: String,
  /// The filter to apply to the query.  Any type that can be serialised to a BSON document can be used.
  pub document: E,
  /// The MongoDB options associated with the request.
  pub options: Option<Options>,
  /// Optional *correlation id* to associate with the metric record created by this action.
  pub correlationId: Option<String>,
  /// Indicate whether to create or skip creating a *metric* document for this operation.
  pub skipMetric: bool
}

impl<E: Serialize> Request<E>
{
  /// Creates a new instance of the `Request` structure with the specified application, database,
  /// collection, and document.
  ///
  /// # Arguments
  ///
  /// * `application` - A string slice that holds the name of the application which is invoking this request.
  /// * `database` - A string slice representing the name of the database.
  /// * `collection` - A string slice specifying the name of the database collection involved in the request.
  /// * `document` - A generic type `E` that represents the document/data associated with the request.
  ///
  /// # Returns
  ///
  /// Returns an instance of the `Request` structure with the provided values and default values for
  /// the following fields:
  ///
  /// * `options` - Set to `None`.
  /// * `correlationId` - Set to `None`.
  /// * `skipMetric` - Set to `false`.
  ///
  /// # Example
  ///
  /// ```
  /// use bson::{doc, Document, oid::ObjectId};
  /// use mongo_service::repository::model::retrieve::Request;
  /// let request: Request<Document> = Request::new("my_app", "my_database", "my_collection", doc!{"_id": ObjectId::new()});
  /// ```
  ///
  /// In this example, a new `Request` instance is created with the application `"my_app"`,
  /// database `"my_database"`, collection `"my_collection"`, and the generic document `my_document`.
  pub fn new(application: &str, database: &str, collection: &str, document: E) -> Self
  {
    Request {application: application.to_string(), database: database.to_string(),
      collection: collection.to_string(), document,
      options: None, correlationId: None, skipMetric: false}
  }

  /// Serializes the current instance into a byte vector.
  ///
  /// This function performs the following steps:
  /// 1. Serializes the current instance (`self`) into a BSON document using the helper function `serialize_to_document`.
  /// 2. Adds an additional key-value pair ("action", `retrieve`) to the serialized document.
  /// 3. Converts the BSON document into a byte vector and returns it.
  ///
  /// # Returns
  /// A `Result` containing the serialized byte vector (`Vec<u8>`) if successful, or an error (`Box<dyn std::error::Error>`) if any step of the serialization process fails.
  ///
  /// # Errors
  /// This function returns an error if:
  /// - The `serialize_to_document` function fails to serialize the object.
  /// - The BSON document cannot be converted to a byte vector.
  pub fn serialise(&self) -> Result<Vec<u8>, Box<dyn std::error::Error>>
  {
    let mut entity = serialize_to_document(self)?;
    entity.insert("action", format!("{:?}", crate::Action::retrieve));
    Ok(entity.to_vec()?)
  }
}

/// A builder to construct a `Request` instance for retrieve operations.
///
/// The `RequestBuilder` struct provides a convenient way to create and configure
/// instances of a retrieve `Request` with the desired parameters. It is a wrapper around
/// the `Request` type and allows for fluent and incremental construction of requests.
///
/// # Type Parameters
/// * `E` - The type representing the query document or filter criteria.
///
/// # Fields
/// * `request` - The underlying `Request` instance that is being built.
pub struct RequestBuilder<E>
{
  /// The underlying `Request` instance that is being built.
  request: Request<E>
}

impl<E: Serialize> RequestBuilder<E>
{
  /// Creates a new `RequestBuilder` instance.
  ///
  /// # Parameters
  /// - `application`: A string slice that holds the name of the application/client using the service.
  /// - `database`: A string slice that holds the name of the database where the request will be executed.
  /// - `collection`: A string slice that specifies the name of the collection within the database.
  /// - `document`: The query document or filter of type `E`.
  ///
  /// # Returns
  /// Returns a new `RequestBuilder` instance with default values.
  pub fn new(application: &str, database: &str, collection: &str, document: E) -> Self
  {
    RequestBuilder {
      request: Request::new(application, database, collection, document)
    }
  }

  /// Sets the MongoDB options for the retrieve operation.
  ///
  /// # Parameters
  /// - `options`: The `Options` to be set for this request.
  ///
  /// # Returns
  /// Returns `self` to allow method chaining.
  pub fn with_options(mut self, options: Options) -> Self
  {
    self.request.options = Some(options);
    self
  }

  /// Sets the correlation ID for the metrics record.
  ///
  /// # Parameters
  /// - `correlation_id`: A string slice representing the correlation ID.
  ///
  /// # Returns
  /// Returns `self` to allow method chaining.
  pub fn with_correlation_id(mut self, correlation_id: &str) -> Self
  {
    self.request.correlationId = Some(correlation_id.to_string());
    self
  }

  /// Sets whether to skip creating a metrics record.
  ///
  /// # Parameters
  /// - `skip`: A boolean indicating whether to skip metrics record creation.
  ///
  /// # Returns
  /// Returns `self` to allow method chaining.
  pub fn skip_metric(mut self, skip: bool) -> Self
  {
    self.request.skipMetric = skip;
    self
  }

  /// Builds and returns the final `Request` instance.
  ///
  /// # Returns
  /// Returns the constructed `Request` object.
  pub fn build(self) -> Request<E>
  {
    self.request
  }
}

#[derive(Debug, Deserialize, Serialize)]
pub enum Response<E>
{
  Result(E),
  Results(Vec<E>)
}
