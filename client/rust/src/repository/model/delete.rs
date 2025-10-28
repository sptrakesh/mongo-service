use bson::{doc, serialize_to_document, Document};
use bson::oid::ObjectId;
use serde::{Deserialize, Serialize};
use crate::repository::model::History;

/// Specifies the options for deleting documents from a database collection.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Options
{
  /// The write concern for the operation.
  pub writeConcern: Option<super::options::WriteConcern>,
  /// The collation to use for text data.
  pub collation: Option<super::options::Collation>
}

/// Represents a request structure used to delete documents from a MongoDB collection.
///
/// # Type Parameters
/// - `M`: Custom metadata type to include with the version history document.
///
/// # Fields
/// - `application`:
///   The name of the application or client initiating the request. This information is
///   included in the service metrics record.
/// - `database`:
///   The name of the MongoDB database on which the operation will be executed.
/// - `collection`:
///   The name of the MongoDB collection on which the operation will be executed.
/// - `document`:
///   A MongoDB document containing the query criteria for deleting matching documents.
///   Note: An empty document, which would delete all documents in the collection, is *not* supported.
/// - `options`:
///   An optional field specifying additional MongoDB options (e.g., write concern, collation)
///   associated with the request.
/// - `metadata`:
///   Optional custom metadata of type `M` that can be included in the version history document created.
/// - `correlationId`:
///   An optional correlation identifier that can be used to associate this request with a
///   specific metrics record.
/// - `skipVersion`:
///   A boolean flag indicating whether to skip creating a *version history* document for this
///   operation. This is useful for non-critical data updates, such as logging operations.
/// - `skipMetric`:
///   A boolean flag indicating whether to skip creating a *metric* document for this operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Request<M>
{
  /// The application/client name.  This is added to the service metrics record.
  pub application: String,
  /// The database against which the operation will be executed.
  pub database: String,
  /// The collection against which the operation will be executed.
  pub collection: String,
  /// The document that contains the query to use to delete matching documents.  An *empty*
  /// document which would delete all documents in the collection is *not* supported.
  pub document: Document,
  /// The MongoDB options associated with the request.
  pub options: Option<Options>,
  /// Custom metadata to add to the version history document created.
  pub metadata: Option<M>,
  /// Optional *correlation id* to associate with the metric record created by this action.
  pub correlationId: Option<String>,
  /// Indicate whether to ignore creating a *version history* document for this insert.  Useful when
  /// creating non-critical data such as logs.
  pub skipVersion: bool,
  /// Indicate whether to create or skip creating a *metric* document for this operation.
  pub skipMetric: bool
}

impl<M: Serialize> Request<M>
{
  /// Constructs a new `Request` object.
  ///
  /// # Parameters
  /// - `application`: A string slice that holds the name of the application/client using the service.
  /// - `database`: A string slice that holds the name of the database where the request will be executed.
  /// - `collection`: A string slice that specifies the name of the collection within the database.
  /// - `entity`: An object of type `E` representing the content/document to be inserted.
  ///
  /// # Returns
  /// Returns an instance of the `Request` struct.
  ///
  /// # Default Values
  /// - `options`: `None`
  /// - `metadata`: `None`
  /// - `correlationId`: `None`
  /// - `action`: Default value is `crate::Action::create` indicating a "create" action.
  /// - `skipVersion`: Default value is `false`.
  /// - `skipMetric`: Default value is `false`.
  ///
  /// # Type Parameter
  /// - `E`: Represents the type of the entity/document being passed into the request.
  pub fn new(application: &str, database: &str, collection: &str, document: Document) -> Self
  {
    Request{application: application.to_string(), database: database.to_string(),
      collection: collection.to_string(), document,
      options: None, metadata: None, correlationId: None,
      skipVersion: false, skipMetric: false}
  }

  /// Serializes the current object into a vector of bytes including an additional "action" field.
  ///
  /// This method first converts the object into a document representation using the `serialize_to_document` function.
  /// It then adds the "action" field with the value corresponding to the `Action::delete` variant
  /// (assumed to be an enum defined in the `crate` module). Finally, it converts the modified document
  /// into a byte vector and returns the result.
  ///
  /// # Returns
  ///
  /// * `Ok(Vec<u8>)` - A vector of bytes representing the serialized object, including the "action" field.
  /// * `Err(Box<dyn std::error::Error>)` - If serialization fails or an unexpected error occurs.
  ///
  /// # Errors
  /// This function will return an error if:
  /// - `serialize_to_document(self)` fails to serialize the object to a document.
  /// - Converting the modified document to a vector of bytes via the `to_vec` method fails.
  pub fn serialise(&self) -> Result<Vec<u8>, Box<dyn std::error::Error>>
  {
    let mut entity = serialize_to_document(self)?;
    entity.insert("action", format!("{:?}", crate::Action::delete));
    Ok(entity.to_vec()?)
  }
}

/// A builder to construct a `Request` instance for delete operations.
///
/// The `RequestBuilder` struct provides a convenient way to create and configure
/// instances of a delete `Request` with the desired parameters. It is a wrapper around
/// the `Request` type and allows for fluent and incremental construction of requests.
///
/// # Type Parameters
/// * `M` - The type representing custom metadata associated with the operation.
///
/// # Fields
/// * `request` - The underlying `Request` instance that is being built.
pub struct RequestBuilder<M>
{
  /// The underlying `Request` instance that is being built.
  request: Request<M>
}

impl<M: Serialize> RequestBuilder<M>
{
  /// Creates a new `RequestBuilder` instance.
  ///
  /// # Parameters
  /// - `application`: A string slice that holds the name of the application/client using the service.
  /// - `database`: A string slice that holds the name of the database where the request will be executed.
  /// - `collection`: A string slice that specifies the name of the collection within the database.
  /// - `document`: A MongoDB document containing the query criteria for deleting matching documents.
  ///
  /// # Returns
  /// Returns a new `RequestBuilder` instance with default values.
  pub fn new(application: &str, database: &str, collection: &str, document: Document) -> Self
  {
    RequestBuilder {
      request: Request::new(application, database, collection, document)
    }
  }

  /// Sets the MongoDB options for the delete operation.
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

  /// Sets the custom metadata for the version history document.
  ///
  /// # Parameters
  /// - `metadata`: The metadata of type `M` to be included in the version history document.
  ///
  /// # Returns
  /// Returns `self` to allow method chaining.
  pub fn with_metadata(mut self, metadata: M) -> Self
  {
    self.request.metadata = Some(metadata);
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

  /// Sets whether to skip creating a version history document.
  ///
  /// # Parameters
  /// - `skip`: A boolean indicating whether to skip version history creation.
  ///
  /// # Returns
  /// Returns `self` to allow method chaining.
  pub fn skip_version(mut self, skip: bool) -> Self
  {
    self.request.skipVersion = skip;
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
  pub fn build(self) -> Request<M>
  {
    self.request
  }
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Response
{
  pub success: Vec<ObjectId>,
  pub failure: Vec<ObjectId>,
  pub history: Vec<History>
}
