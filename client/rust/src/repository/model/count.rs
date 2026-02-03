use bson::{serialize_to_document, Bson};
use serde::{Deserialize, Serialize};

/// Options for a `count` operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Options
{
  /// The collation to use for text data.
  pub collation: Option<super::options::Collation>,
  pub readPreference: Option<super::options::ReadPreference>,
  /// The index to use. Specify either the index name as a string or the index specification document.
  pub hint: Option<Bson>,
  /// Specifies the time limit in milliseconds for the count operation to run before timing out.
  pub maxTime: Option<u64>,
  /// The maximum number of matching documents to return
  pub limit: Option<u64>,
  /// The number of matching documents to skip before returning results.
  pub skip: Option<u64>
}

/// Request for a count operation.
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
  pub fn new(application: &str, database: &str, collection: &str, document: E) -> Self
  {
    Request {application: application.to_string(), database: database.to_string(),
      collection: collection.to_string(), document,
      options: None, correlationId: None, skipMetric: false}
  }

  pub fn serialise(&self) -> Result<Vec<u8>, Box<dyn std::error::Error>>
  {
    let mut entity = serialize_to_document(self)?;
    entity.insert("action", format!("{:?}", crate::Action::count));
    Ok(entity.to_vec()?)
  }
}

/// A builder to construct a `Request` instance for count operations.
///
/// The `RequestBuilder` struct provides a convenient way to create and configure
/// instances of a count `Request` with the desired parameters. It is a wrapper around
/// the `Request` type and allows for fluent and incremental construction of requests.
///
/// # Type Parameters
/// * `E` - The type representing the filter document or query criteria.
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
  /// - `document`: The filter document or query of type `E`.
  ///
  /// # Returns
  /// Returns a new `RequestBuilder` instance with default values.
  pub fn new(application: &str, database: &str, collection: &str, document: E) -> Self
  {
    RequestBuilder {
      request: Request::new(application, database, collection, document)
    }
  }

  /// Sets the MongoDB options for the count operation.
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

/// Response from the service for a successful count operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Response
{
  /// The number of matching documents.
  pub count: u64
}
