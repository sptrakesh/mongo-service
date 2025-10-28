#![allow(non_snake_case)]

use bson::{doc, serialize_to_document, oid::ObjectId};
use serde::{Deserialize, Serialize};

/// Specifies the options for inserting documents into a database collection.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Options
{
  /// The write concern for the operation.
  pub writeConcern: Option<super::options::WriteConcern>,
  /// If `true`, ignores any schema validation rules specified on the collection.
  pub bypassValidation: Option<bool>,
  /// If `true`, continues attempting to insert remaining documents when an individual insert fails.
  /// Only relevant when inserting a batch of documents.
  pub ordered: Option<bool>
}

/// A structure that represents the information required to perform an insert operation on a database collection.
///
/// # Type Parameters
/// - `E`: Represents the type of the document that will be inserted into the collection.
/// - `M`: Represents the type of custom metadata that can be added to the version history document.
///
/// # Fields
/// - `application`:
///   The name of the application or client issuing the request. This is typically used
///   for tracking and is included in the service metrics record for observability purposes.
///
/// - `database`:
///   The name of the target database where the requested operation will be executed.
///
/// - `collection`:
///   The name of the target collection within the database where the requested operation will be executed.
///
/// - `document`:
///   The document of type `E` that is to be inserted into the collection. It is mandatory
///   that the serialized BSON representation of this document includes an `_id` field of
///   type `ObjectId`.
///
/// - `options`:
///   Optional settings of type [`Options`] that provide additional configuration for the
///   database operation, such as write or read preferences.
///
/// - `metadata`:
///   Optional custom metadata of type `M` to include in the version history document
///   associated with the operation. This allows attaching additional contextual information.
///
/// - `correlationId`:
///   An optional string identifier used to associate this request with a specific metric or
///   trace record. This is useful for tracking the execution of the operation across
///   different systems or services.
///
/// - `skipVersion`:
///   A flag indicating whether to skip creating a version history document for this insert.
///   When set to `true`, no version history document will be created, which can be useful
///   for non-critical data, such as logs or temporary records.
///
/// - `skipMetric`:
///   A flag indicating whether to create a metric document for this operation. When set
///   to `true`, metrics associated with this operation will not be recorded.
/// 
/// # Notes
/// - Ensure the `document` field contains an `_id` of type `ObjectId` to avoid
///   runtime errors during BSON serialization.
/// - Utilize `skipVersion` and `skipMetric` flags judiciously to balance data storage
///   and operational visibility.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Request<E, M>
{
  /// The application/client name.  This is added to the service metrics record.
  pub application: String,
  /// The database against which the operation will be executed.
  pub database: String,
  /// The collection against which the operation will be executed.
  pub collection: String,
  /// The document that is to be inserted into the collection.  The serialised BSON **must** contain an
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
  pub fn new(application: &str, database: &str, collection: &str, entity: E) -> Self
  {
    Request{application: application.to_string(), database: database.to_string(),
      collection: collection.to_string(), document: entity,
      options: None, metadata: None, correlationId: None,
      skipVersion: false, skipMetric: false}
  }

  /// Serializes the current object into BSON bytes, incorporating additional metadata and structure.
  ///
  /// # Returns
  ///
  /// * `Result<Vec<u8>, Box<dyn std::error::Error>>` - A vector of bytes representing the serialized object, or an error if serialization fails.
  ///
  /// # Functionality
  ///
  /// 1. Serializes the current object into a document format using `serialize_to_document`.
  /// 2. Inserts an "action" key into the serialized document, containing the string representation of the `Action::create` enum variant.
  /// 3. Constructs a `payload` document that includes:
  ///    - A "filter" sub-document, which references the object's `_id`.
  ///    - A "replace" sub-document that contains the current entity.
  /// 4. Updates the main serialized document by removing the existing "document" field (if any) and replacing it with the newly constructed `payload`.
  /// 5. Converts the updated document into a byte vector and returns it.
  ///
  /// # Errors
  ///
  /// This function accumulates various error sources and returns them as `Box<dyn std::error::Error>`. Errors may occur in:
  ///
  /// - `serialize_to_document` if the current object cannot be serialized.
  /// - Accessing the `_id` field using `get_object_id`.
  /// - The `.to_vec()` method if converting the document to bytes fails.
  pub fn serialise(&self) -> Result<Vec<u8>, Box<dyn std::error::Error>>
  {
    let mut entity = serialize_to_document(self)?;
    entity.insert("action", format!("{:?}", crate::Action::create));
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
/// * `E` - The type specifying the endpoint or target of the request.
/// * `M` - The type representing the message or payload of the request.
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
  pub fn new(application: &str, database: &str, collection: &str, entity: E) -> Self
  {
    RequestBuilder{request: Request::<E, M>::new(application, database, collection, entity)}
  }

  pub fn with_options(mut self, options: Options) -> Self
  {
    self.request.options = Some(options);
    self
  }

  pub fn with_metadata(mut self, metadata: M) -> Self
  {
    self.request.metadata = Some(metadata);
    self
  }

  pub fn with_correlation_id(mut self, correlationId: String) -> Self
  {
    self.request.correlationId = Some(correlationId);
    self
  }

  pub fn with_skip_version(mut self, skipVersion: bool) -> Self
  {
    self.request.skipVersion = skipVersion;
    self
  }

  pub fn with_skip_metric(mut self, skipMetric: bool) -> Self
  {
    self.request.skipMetric = skipMetric;
    self
  }

  pub fn build(self) -> Request<E, M>
  {
    self.request
  }
}

/// Response from the service for a successful insert operation when no version history was specified.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct ResponseNoHistory
{
  /// The `_id` of the entity that was created.
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId,
  /// Indicates the insert was performed without creating a version history document.
  /// Always `true`.
  pub skipVersion: bool
}

/// Response variant for the `create` action.
#[derive(Debug)]
pub enum Response
{
  /// Response from the service for a successful insert operation with version history.
  WithHistory(super::History),
  /// Response from the service for a successful insert operation without version history.
  NoHistory(ResponseNoHistory)
}
