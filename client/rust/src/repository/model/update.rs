use bson::{doc, serialize_to_document, Array};
use serde::{Deserialize, Serialize};

/// Specifies the options for updating documents into a database collection.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Options
{
  /// The write concern for the operation.
  pub writeConcern: Option<super::options::WriteConcern>,
  /// The collation to use for text data.
  pub collation: Option<super::options::Collation>,
  /// An array of filter documents that determine which array elements to modify for an update operation on an array field.
  pub arrayFilters: Option<Array>,
  /// Specifies the time limit in milliseconds for the update operation to run before timing out.
  pub maxTimeMS: Option<u64>,
  /// If set to `true`, updates *multiple* documents that meet the query criteria. If set to `false`,
  /// updates *one* document. The default value is `false`.
  pub multi: Option<bool>,
  /// If `true`, inserts a new document if no document matches the filter.  If `false`, an error is returned
  pub upsert: Option<bool>,
  /// If `true`, ignores any schema validation rules specified on the collection.  Only applies if
  /// `upsert` is set to `true`.
  pub bypassDocumentValidation: Option<bool>,
}

/// Represents a request structure used to update a document in a database collection.
///
/// This struct is generic over `E` (the type of the document to update) and `M` (the type of custom
/// metadata associated with the operation). It supports serialization and deserialization, as well
/// as cloning and debugging.
///
/// # Fields
///
/// * `application` - The name of the application/client making the request. This is used to record
///   service metrics.
/// * `database` - The name of the database where the operation will be performed.
/// * `collection` - The name of the collection where the operation will take place.
/// * `document` - The document to be updated in the collection. This must be serialized as BSON
///   and **must** include an `_id` field of type `ObjectId`.
/// * `options` - Optional MongoDB options to customise the operation.
/// * `metadata` - Optional custom metadata to include in the version history document created as
///   part of this operation.
/// * `correlationId` - Optional correlation ID to associate with the metrics record created for
///   this action.
/// * `skipVersion` - A flag to indicate whether to bypass the creation of a version history
///   document for this operation. This is convenient when writing non-critical data, such as logs.
/// * `skipMetric` - A flag to indicate whether to skip creating a metrics record for this
///   operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Request<E, M>
{
  /// The application/client name.  This is added to the service metrics record.
  pub application: String,
  /// The database against which the operation will be executed.
  pub database: String,
  /// The collection against which the operation will be executed.
  pub collection: String,
  /// The document that is to be updated in the collection.  The serialised BSON **must** contain an
  /// `_id` field of type ObjectId.
  pub document: E,
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

impl<E: Serialize, M: Serialize> Request<E, M>
{
  /// Constructs a new `Request` object.
  ///
  /// # Parameters
  /// - `application`: A string slice that holds the name of the application/client using the service.
  /// - `database`: A string slice that holds the name of the database where the request will be executed.
  /// - `collection`: A string slice that specifies the name of the collection within the database.
  /// - `entity`: An object of type `E` representing the content/document to be updated.
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
  pub fn new(application: &str, database: &str, collection: &str, entity: E) -> Self
  {
    Request {application: application.to_string(), database: database.to_string(),
      collection: collection.to_string(), document: entity,
      options: None, metadata: None, correlationId: None,
      skipVersion: false, skipMetric: false}
  }

  /// Serializes the current instance into a vector of bytes compatible with BSON.
  ///
  /// This function first serializes the current instance into a BSON document. It then:
  /// 1. Inserts the action type with the value `Action::update` into the document, under the key `"action"`.
  /// 2. Creates a payload containing a filter for the `_id` field and its replacement data.
  /// 3. Inserts the payload under the key `"document"`.
  ///
  /// During the process, the original `"document"` field in the entity (if any) is removed and replaced by the new constructed payload.
  ///
  /// # Returns
  /// - On success, it returns a `Vec<u8>` representing the serialized BSON data.
  /// - On failure, it returns a boxed error indicating the reason for the failure.
  ///
  /// # Errors
  /// - Returns an error if the `_id` field is missing or cannot be retrieved from the serialized document.
  /// - Returns an error if the BSON document cannot be serialized into a vector of bytes.
  pub fn serialise(&self) -> Result<Vec<u8>, Box<dyn std::error::Error>>
  {
    let mut entity = serialize_to_document(self)?;
    entity.insert("action", format!("{:?}", crate::Action::update));
    let payload = doc!{
      "filter": doc!{"_id": entity.get_document("document").unwrap().get_object_id("_id")?},
      "replace": entity.get_document("document").unwrap()
    };
    entity.remove("document");
    entity.insert("document", payload);
    Ok(entity.to_vec()?)
  }
}

/// A builder to construct a `Request` instance.
///
/// The `RequestBuilder` struct provides a convenient way to create and configure
/// instances of a `Request` with the desired parameters. It is a wrapper around
/// the `Request` type and allows for fluent and incremental construction of requests.
///
/// # Type Parameters
/// * `E` - The type of the document to be updated.
/// * `M` - The type representing custom metadata associated with the operation.
///
/// # Fields
/// * `request` - The underlying `Request` instance that is being built.
pub struct RequestBuilder<E, M>
{
  /// The underlying `Request` instance that is being built.
  request: Request<E, M>
}

impl<E: Serialize, M: Serialize> RequestBuilder<E, M>
{
  /// Creates a new `RequestBuilder` instance.
  ///
  /// # Parameters
  /// - `application`: A string slice that holds the name of the application/client using the service.
  /// - `database`: A string slice that holds the name of the database where the request will be executed.
  /// - `collection`: A string slice that specifies the name of the collection within the database.
  /// - `entity`: An object of type `E` representing the document to be updated.
  ///
  /// # Returns
  /// Returns a new `RequestBuilder` instance with default values.
  pub fn new(application: &str, database: &str, collection: &str, entity: E) -> Self
  {
    RequestBuilder {
      request: Request::new(application, database, collection, entity)
    }
  }

  /// Sets the MongoDB options for the update operation.
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
  pub fn build(self) -> Request<E, M>
  {
    self.request
  }
}

/// Response from the service for a successful update operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct ResponseWithHistory<E>
{
  pub document: E,
  pub history: super::History
}

/// Response from the service for a successful update operation when no version history was specified.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct ResponseNoHistory
{
  /// Indicates the insert was performed without creating a version history document.
  /// Always `true`.
  pub skipVersion: bool
}

/// Response variant for the `update` action.
#[derive(Debug)]
pub enum Response<E>
{
  /// Response from the service for a successful update operation with version history.
  WithHistory(ResponseWithHistory<E>),
  /// Response from the service for a successful update operation without version history.
  NoHistory(ResponseNoHistory)
}
