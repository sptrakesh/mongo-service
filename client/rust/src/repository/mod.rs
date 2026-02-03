use bson::{deserialize_from_document, doc, Bson, Document, oid::ObjectId};
use std::error::Error;
use serde::{Deserialize, Serialize};

pub mod model;

/// Creates an entity in the database.
///
/// # Parameters
/// - `request`: A `model::create::Request<E, M>` object that contains the data to be sent.
///
/// # Returns
/// - `Ok(model::create::Response)`: A successful response, which can either be:
///    - `Response::WithHistory`: Indicates that the entity was created with history, containing a deserialized `model::History` object.
///    - `Response::NoHistory`: Indicates that the entity was created without history, containing a deserialized `model::create::ResponseNoHistory` object.
/// - `Err(Box<dyn Error>)`: If an error occurs during serialization, request execution, response parsing, or if the response contains an error.
///
/// # Errors
/// - Returns an error if:
///   - The request serialization (`serialise`) fails.
///   - The response from the `cpp::execute` function is an error.
///   - The response data is not valid or is missing required keys.
///   - The response contains an "error" key indicating an issue reported by `mongo-service`.
///   - The response is invalid or does not match the expected keys ("entity", "_id", or "error").
pub fn create<E: Serialize, M: Serialize>(request: model::create::Request<E, M>) -> Result<model::create::Response, Box<dyn Error>>
{
  let bytes = request.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("entity") { Ok(model::create::Response::WithHistory(deserialize_from_document::<model::History>(d)?)) }
  else if d.contains_key("_id") { Ok(model::create::Response::NoHistory(deserialize_from_document::<model::create::ResponseNoHistory>(d)?)) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// This function is a generic implementation to retrieve data from a request object
/// and return it in structured format after deserialization.
///
/// # Type Parameters
/// - `D`: A type that implements the `Serialize` trait, representing the data structure
///   to be serialized for the given request.
/// - `E`: A type that implements the `Deserialize` trait, representing the data structure
///   to be deserialized from the response.
///
/// # Arguments
/// - `request`: A `model::retrieve::Request<D>` object that contains the data to be serialized
///   and sent to the backend service.
///
/// # Returns
/// - `Ok(model::retrieve::Response<E>)`: Returns a structured response object containing the requested data,
///   which can either be a single result (`Response::Result`) or multiple results (`Response::Results`),
///   depending on the response format.
/// - `Err(Box<dyn Error>)`: Returns an error in the following scenarios:
///   - If serialization of the request fails.
///   - If the response from the backend service is an error.
///   - If the response does not match one of the expected formats (e.g., missing "result", "results", or "error" keys).
///
/// # Errors
/// - If the response contains an "error" key, the associated string value will be converted into an error and returned.
/// - If the backend service returns an invalid or unexpected response format, an error with the message
///   "Invalid response from mongo-service" will be raised.
///
/// # Implementation Details
/// 1. The function serializes the request using the `serialise` method.
/// 2. It sends the serialized request to the backend service using `crate::client::cpp::execute`.
/// 3. If the response contains the key:
///    - `"result"`: It deserializes the single result into type `E` and returns it.
///    - `"results"`: It deserializes a list of results into type `E` and returns them as a vector.
///    - `"error"`: It extracts the error message and returns it as an error result.
///    - Any other case: The response is considered invalid, and an error is returned.
///
/// # Example
/// ```rust
/// use mongo_service::repository::{find, model::retrieve::{Request, Response}};
/// use serde::{Serialize, Deserialize};
/// use std::error::Error;
///
/// #[derive(Serialize)]
/// struct MyRequest {
///     field: String,
/// }
///
/// #[derive(Deserialize, Debug)]
/// struct MyResponse {
///     id: u32,
///     name: String,
/// }
///
/// fn invoke_find_function() -> Result<(), Box<dyn Error>> {
///     let request = Request::<MyRequest>::new("rust-app", "test", "test", MyRequest { field: "example".to_string() });
///     let response: Response<MyResponse> = find(request)?;
///
///     match response {
///         Response::Result(result) => println!("{:?}", result),
///         Response::Results(results) => results.iter().for_each(|r| println!("{:?}", r)),
///     }
///
///     Ok(())
/// }
/// ```
///
/// This is a high-level utility for interacting with the backend service, leveraging serialization/deserialization
/// to abstract away raw data handling.
pub fn find<D: Serialize, E: for <'de> Deserialize<'de>>(request: model::retrieve::Request<D>) -> Result<model::retrieve::Response<E>, Box<dyn Error>>
{
  let bytes = request.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("result")
  {
    Ok(model::retrieve::Response::Result(deserialize_from_document::<E>(d)?))
  }
  else if d.contains_key("results")
  {
    let a = d.get_array("results").unwrap();
    Ok(model::retrieve::Response::Results(a.iter().map(|x| deserialize_from_document::<E>(x.as_document().unwrap().clone()).unwrap()).collect()))
  }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Retrieves a document from the specified database and collection by its ObjectId.
///
/// # Arguments
///
/// * `id` - The `ObjectId` of the document to retrieve.
/// * `application` - A string slice specifying the application context.
/// * `database` - The name of the database where the document resides.
/// * `collection` - The name of the collection where the document resides.
/// * `options` - An optional `model::retrieve::Options` object containing additional options for the request.
///
/// # Returns
///
/// Returns a `Result` which is:
/// * `Ok(E)` - The document deserialized into the generic type `E`, if the operation succeeds.
/// * `Err(Box<dyn Error>)` - An error if the operation fails, including:
///     - Issues with the request serialization.
///     - Errors returned from the `mongo-service`.
///     - Invalid response structure from `mongo-service`.
///
/// # Type Parameters
///
/// * `E` - A type that implements `Deserialize`. Represents the expected structure of the returned document.
///
/// # Errors
///
/// This function will return an error if:
/// * The `mongo-service` responds with an error.
/// * The response does not contain a valid `result` or `error` field.
/// * There is an issue with deserializing the document into type `E`.
/// * The internal client communication (`cpp::execute`) fails.
///
/// # Example
///
/// ```rust, no_run
/// use bson::oid::ObjectId;
/// use mongo_service::repository::by_id;
/// use serde::Deserialize;
///
/// #[derive(Debug, Deserialize)]
/// struct MyDocument {
///     field_a: String,
///     field_b: i32,
/// }
///
/// let id = ObjectId::parse_str("64bd1f7dfb57a9f6032a4462").unwrap();
/// let result: MyDocument = by_id(id, "my_app", "my_db", "my_collection", None).unwrap();
/// println!("Retrieved document: {:?}", result);
/// ```
pub fn by_id<E: for <'de> Deserialize<'de>>(id: ObjectId, application: &str, database: &str, collection: &str,
    options: Option<model::retrieve::Options>) -> Result<E, Box<dyn Error>>
{
  let mut request = model::retrieve::Request::<Document>::new(application, database, collection, doc!{"_id": id});
  if options.is_some() { request.options = options; }
  let bytes = request.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("result") { Ok(deserialize_from_document::<E>(d.get_document("result").unwrap().clone())?) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Filters documents from a MongoDB collection based on specified criteria and options.
///
/// # Type Parameters:
/// - `E`: The type to which the resulting documents will be deserialized.
///         This type must implement the `Deserialize` trait for deserialization.
///
/// # Arguments:
/// - `filter` (`Document`): The MongoDB filter query used to filter documents.
/// - `application` (`&str`): The name of the application schema for database context.
/// - `database` (`&str`): The name of the database to query.
/// - `collection` (`&str`): The name of the collection from which to query documents.
/// - `descending` (`bool`): Whether to sort the `_id` field in descending (`true`) or ascending (`false`) order.
///      Only used when no options are provided.
/// - `options` (`Option<model::retrieve::Options>`): Options for the retrieve operation.
///     If `None`, the default options are used.
///     If `Some(options)`, the provided options are used.
///     The default options are:
///     - `limit` (`None`): No limit on the number of documents returned.
///     - `sort` (`Some(doc!{"_id": if descending { -1 } else { 1 }})`): Sort the results by `_id` in descending (`true`) or ascending (`false`) order.
///     - `skip` (`None`): No skip in the results.
///     - `projection` (`None`): No projection in the results.
///     - `hint` (`None`): No hint in the results.
///     - `comment` (`None`): No comment in the results.
///
/// # Returns:
/// - `Ok(model::retrieve::Response<E>)`: Returns a response which can either be:
///   - `model::retrieve::Response::Result`: A unique document matching the filter.
///   - `model::retrieve::Response::Results`: A list of documents matching the filter.
/// - `Err(Box<dyn Error>)`: Returns an error if the operation fails, such as:
///   - Issues with serialization or deserialization.
///   - Errors returned from the MongoDB service.
///   - An invalid response structure from the database service.
///
/// # Errors:
/// - If the `filter` or response cannot be serialized/deserialized, an error is returned.
/// - If the response from the MongoDB backend service contains an error or invalid structure, an error is returned.
///
/// # Example:
/// ```rust, no_run
/// use bson::{doc, Document};
/// use mongo_service::repository;
/// use serde::Deserialize;
///
/// #[derive(Debug, Deserialize)]
/// struct MyDocument {
///     field1: String,
///     field2: i32,
/// }
///
/// let filter = doc! { "field1": "value" };
/// let result = repository::filter::<MyDocument>(
///     filter,
///     "my_application",
///     "my_database",
///     "my_collection",
///     false,
///     None
/// );
///
/// match result {
///     Ok(response) => match response {
///         repository::model::retrieve::Response::Result(doc) => println!("Got single document: {:?}", doc),
///         repository::model::retrieve::Response::Results(docs) => println!("Got documents: {:?}", docs),
///     },
///     Err(e) => eprintln!("Error occurred: {}", e),
/// }
/// ```
///
/// # Notes:
/// - Sorting is applied on the `_id` field.
/// - If `descending` is `true`, the results are sorted in descending order; otherwise, they're sorted in ascending order.
/// - Always ensure consistent error handling based on the response context.
pub fn filter<E: for <'de> Deserialize<'de>>(filter: Document, application: &str,
  database: &str, collection: &str, descending: bool, options: Option<model::retrieve::Options>) -> Result<model::retrieve::Response<E>, Box<dyn Error>>
{
  let mut request = model::retrieve::Request::<Document>::new(application, database, collection, filter);
  if options.is_some() { request.options = options; }
  else
  {
    if descending { request.options = Some(model::retrieve::Options::new()); }
    else
    {
      let mut opts = model::retrieve::Options::new();
      opts.sort = Some(doc!{"_id": if descending { -1 } else { 1 }});
      request.options = Some(opts);
    }
  }
  find(request)
}

/// A function that retrieves a filtered list of documents from a MongoDB collection based on the provided field and value criteria.
///
/// # Type Parameters
/// - `E`: A type that implements `Deserialize<'de>` for deserializing documents into the specified type.
///
/// # Parameters
/// - `field`: A string slice (`&str`) representing the field name to filter the documents by.
/// - `value`: A `Bson` value that specifies the value to match for the specified field.
/// - `application`: A string slice (`&str`) representing the name of the application (likely used to identify the MongoDB client or connection settings).
/// - `database`: A string slice (`&str`) representing the name of the database to query.
/// - `collection`: A string slice (`&str`) representing the name of the collection to search within.
/// - `descending`: A `bool` indicating whether the results should be sorted in descending order (true) or ascending order (false).
///      Only useful when no options are passed in.
/// - `options`: Options for the retrieve operation.
///
/// # Returns
/// - `Result<Vec<E>, Box<dyn Error>>`: On success, returns a `Vec` of deserialised documents of type `E`.
///   If there is an error during the operation (e.g., a MongoDB or deserialization error), it returns an error encapsulated in a `Box<dyn Error>`.
///
/// # Behavior
/// - The function constructs a MongoDB filter query document with the specified field and value.
/// - Calls the `filter` function (assumed to handle the query execution and result deserialization) with the provided parameters.
/// - Supports pagination or size-limiting using the `limit` parameter.
/// - Allows sorting the results in either descending or ascending order based on the `descending` flag.
///
/// # Errors
/// - Returns an error if the underlying `filter` function fails to execute the query or if deserialization into the specified type `E` fails.
///
/// # Example
/// ```rust, no_run
/// use bson::{doc, Bson};
/// use mongo_service::repository::filter;
/// use serde::Deserialize;
/// use std::error::Error;
///
/// #[derive(Deserialize, Debug)]
/// struct MyDocument {
///     _id: String,
///     name: String,
///     age: i32,
/// }
///
/// let field = "name";
/// let value = Bson::String("john_doe".to_string());
/// let application = "my_app";
/// let database = "my_database";
/// let collection = "users";
/// let descending = true;
///
/// match mongo_service::repository::property::<MyDocument>(field, value, application, database, collection, descending, None) {
///     Ok(results) => {
///          match results
///          {
///            mongo_service::repository::model::retrieve::Response::<MyDocument>::Results(docs) =>
///            {
///              for doc in &docs { println!("{:?}", doc); }
///            }
///            _ => panic!("Unexpected response")
///          }
///     }
///     Err(e) => {
///         eprintln!("Error fetching documents: {}", e);
///     }
/// }
/// ```
pub fn property<E: for <'de> Deserialize<'de>>(field: &str, value: Bson, application: &str,
  database: &str, collection: &str, descending: bool, options: Option<model::retrieve::Options>) -> Result<model::retrieve::Response<E>, Box<dyn Error>>
{
  filter(doc!{field: value}, application, database, collection, descending, options)
}

/// Updates an entity in the database using the provided request.
///
/// The update function takes a request object containing the entity data and metadata, serializes it,
/// and sends it to the database service for execution. The function processes the response returned
/// by the service and deserializes it into the appropriate response type.
///
/// # Type Parameters
/// * `E`: The type of the entity being updated. It must implement both `Deserialize` and `Serialize` traits.
/// * `M`: The type of the metadata associated with the update request. It must implement the `Serialize` trait.
///
/// # Arguments
/// * `request` - An instance of `model::update::Request<E, M>` containing the entity and metadata for the update operation.
///
/// # Returns
/// * `Ok(model::update::Response<E>)` - If the update operation succeeds, it returns a response wrapped in the `Ok` variant:
///     - `Response::WithHistory`: When the response contains the entire update history of the entity.
///     - `Response::NoHistory`: When the response only includes the updated entity without the history.
/// * `Err(Box<dyn Error>)` - If the update operation fails, it returns an error wrapped in the `Err` variant. Some possible error cases include:
///     - Serialization failure of the request.
///     - Execution or communication errors with the database service.
///     - Malformed or unexpected response from the database service.
///
/// # Errors
/// The function may return the following error types:
/// * Serialization and deserialization errors related to the request or the response.
/// * Communication or execution failure with the database service.
/// * Errors reported by the database service in the response (e.g., "error" key in the response document).
/// * A generic error if the response from the database service is invalid or cannot be parsed.
///
/// # Example
/// ```rust, no_run
/// use mongo_service::repository::{update, model::update::Request, model::update::Response};
/// use bson::oid::ObjectId;
/// use serde::{Deserialize, Serialize};
///
/// #[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
/// pub struct Metadata
/// {
///   pub year: String,
///   pub month: String,
/// }
///
/// #[derive(Debug, Deserialize, Serialize)]
/// struct MyDocument {
///   _id: ObjectId,
///   field1: String,
///   field2: i32,
/// }
///
/// let entity = MyDocument{_id: ObjectId::new(), field1: "value1".to_string(), field2: 10};
/// let metadata = Metadata{year: "2020".to_string(), month: "01".to_string()};
/// let mut request = Request::new("rust-test", "test", "test", entity);
/// request.metadata = Some(metadata);
/// match update(request) {
///     Ok(Response::WithHistory(response)) => {
///         println!("Update succeeded. Historical data: {:?}", response);
///     }
///     Ok(Response::NoHistory(response)) => {
///         println!("Update succeeded. No historical data: {:?}", response);
///     }
///     Err(e) => {
///         eprintln!("Update failed: {}", e);
///     }
/// }
/// ```
///
/// # Note
/// The function assumes a specific response structure from the database service:
/// * If the response contains a key `"document"`, it is considered to indicate a success with historical data.
/// * If the response contains a key `"skipVersion"`, it is considered to indicate a success without historical data.
/// * If the response contains a key `"error"`, it is treated as a failure and the error message is extracted.
/// * In case of any other structure, the function returns a generic error indicating an invalid response.
pub fn update<E: for <'de> Deserialize<'de> + Serialize, M: Serialize>(request: model::update::Request<E, M>) -> Result<model::update::Response<E>, Box<dyn Error>>
{
  let bytes = request.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("document") { Ok(model::update::Response::WithHistory(deserialize_from_document::<model::update::ResponseWithHistory<E>>(d)?)) }
  else if d.contains_key("skipVersion") { Ok(model::update::Response::NoHistory(deserialize_from_document::<model::update::ResponseNoHistory>(d)?)) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Deletes a resource based on the provided request model and returns the response or an error.
///
/// This function performs the following steps:
/// 1. Serialises the given `delete` request object to bytes.
/// 2. Sends the serialised request via the `cpp` client to execute the deletion operation.
/// 3. Parses the response into a `Document` and determines its validity.
/// 4. Returns the appropriate response or error based on the content of the document.
///
/// # Type Parameters
/// - `M`: A model type implementing the `Serialize` trait, representing the type of the delete request.
///
/// # Arguments
/// - `request`: A `model::delete::Request<M>` instance that contains the deletion parameters.
///
/// # Returns
/// - `Ok(model::delete::Response)`: If the deletion is successful and the response is valid.
/// - `Err(Box<dyn Error>)`: If serialization, execution, parsing, or any other step fails. Specific errors include:
///   - Serialization failure.
///   - An invalid response from the underlying `mongo-service`.
///   - An error response from the service indicating a problem during the deletion.
///
/// # Errors
/// - Returns an error if serialization of the request fails.
/// - Returns an error if the underlying `cpp` client reports an execution failure.
/// - Returns an error if the response document does not contain expected keys (`success` or `error`).
/// - Returns an error if the response indicates an unsuccessful operation, with details extracted from the error.
///
/// # Example
/// ```rust, no_run
/// use bson::{doc, Document, oid::ObjectId};
/// use mongo_service::repository::{delete, model::delete::Request, model::delete::Response};
///
/// let request = Request::<Document>::new("rust-test", "test", "test", doc!{"_id": ObjectId::new()});
///
/// match delete(request) {
///     Ok(response) => println!("Deletion successful: {:?}", response),
///     Err(e) => println!("Failed to delete: {}", e),
/// }
/// ```
pub fn delete<M: Serialize>(request: model::delete::Request<M>) -> Result<model::delete::Response, Box<dyn Error>>
{
  let bytes = request.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("success") { Ok(deserialize_from_document::<model::delete::Response>(d)?) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Deletes a document from a specified MongoDB collection by its ObjectId.
///
/// # Arguments
///
/// * `id` - The `ObjectId` of the document to be deleted.
/// * `application` - A string slice that specifies the name of the application.
/// * `database` - A string slice that specifies the name of the database.
/// * `collection` - A string slice that specifies the name of the collection.
///
/// # Returns
///
/// * `Ok(model::delete::Response)` - If the document is successfully deleted, it returns
///   the response model containing the details of the operation.
/// * `Err(Box<dyn Error>)` - If an error occurs during the deletion process, it returns
///   a boxed error type.
///
/// # Example
///
/// ```rust, no_run
/// use bson::oid::ObjectId;
/// use mongo_service::repository::delete_by_id;
///
/// let id = ObjectId::parse_str("645c97cfb2fedf03e6b7b0d1").unwrap();
/// let application = "my_app";
/// let database = "my_database";
/// let collection = "my_collection";
///
/// match delete_by_id(id, application, database, collection) {
///     Ok(response) => println!("Delete successful: {:?}", response),
///     Err(e) => eprintln!("Error occurred: {:?}", e),
/// }
/// ```
///
/// # Errors
///
/// This function will return an error if:
/// - The MongoDB client fails to process the deletion request.
/// - There is an issue connecting to the database or collection.
/// - The `id` does not exist in the specified collection.
pub fn delete_by_id(id: ObjectId, application: &str, database: &str, collection: &str) -> Result<model::delete::Response, Box<dyn Error>>
{
  let req = model::delete::Request::<Document>::new(application, database,
    collection, doc!{"_id": id});
  delete(req)
}

pub fn count<M: Serialize>(application: &str, database: &str, collection: &str, filter: M) -> Result<model::count::Response, Box<dyn Error>>
{
  let req = model::count::Request::<M>::new(application, database, collection, filter);
  let bytes = req.serialise()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("count") { Ok(deserialize_from_document::<model::count::Response>(d)?) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Retrieves a specific version history document from the database.
///
/// # Type Parameters
/// - `E`: A generic type representing the structure of the deserialized version history.
///         It must implement the `Deserialize` trait for all lifetimes (`for<'de>`).
///
/// # Parameters
/// - `id`: The `ObjectId` of the document whose version history is to be retrieved.
/// - `application`: A string slice representing the application the document is associated with.
/// - `database`: A string slice representing the name of the database.
/// - `collection`: A string slice representing the name of the collection.
///
/// # Returns
/// - `Ok`: Returns a `model::VersionHistory<E>` instance if the operation is successful.
/// - `Err`: Returns a boxed dynamic error (`Box<dyn Error>`) if the operation fails for any reason:
///     - If the request fails to execute.
///     - If the response contains an error field.
///     - If the response is invalid or cannot be deserialized.
///
/// # Errors
/// - Returns an error if:
///     - The MongoDB service responds with an error in the `error` field.
///     - The service response is invalid or malformed.
///     - An issue occurs while processing the request or response.
///
/// # Example
/// ```rust, no_run
/// use bson::oid::ObjectId;
/// use mongo_service::repository::{version_history_document, model::VersionHistory};
/// use serde::Deserialize;
///
/// #[derive(Debug, Deserialize)]
/// struct MyDocument {
///     field_a: String,
///     field_b: i32,
/// }
///
/// let id = ObjectId::parse_str("64c1234abcd1234ef56789ab").unwrap();
/// let application = "example_app";
/// let database = "example_db";
/// let collection = "example_collection";
///
/// match version_history_document::<VersionHistory<MyDocument>>(id, application, database, collection) {
///     Ok(version_history) => println!("Version history: {:?}", version_history),
///     Err(e) => eprintln!("Error retrieving version history: {}", e),
/// }
/// ```
///
/// # Notes
/// - The function assumes the existence of a `mongo-service` and relies on successful communication
///   with it.
/// - The underlying client communication is assumed to be synchronous or blocking in nature.
/// - The `deserialize_from_document` function is expected to convert the BSON document
///   into the target structured type.
///
/// # Dependencies
/// - The `bson` crate for handling BSON documents.
/// - The `Deserialize` trait for mapping BSON to strongly-typed structures.
/// - An external client (`crate::client::cpp::execute`) for interacting with the service.
///
/// # See Also
/// - [`Deserialize`](https://docs.rs/serde/latest/serde/trait.Deserialize.html)
pub fn version_history_document<E: for <'de> Deserialize<'de>>(id: ObjectId, application: &str,
  database: &str, collection: &str) -> Result<model::VersionHistory<E>, Box<dyn Error>>
{
  let request = doc!{"application": application, "database": database, "collection": collection,
    "action": format!("{:?}", crate::Action::retrieve), "document": doc!{"_id": id}};
  let bytes = request.to_vec()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("result") { Ok(deserialize_from_document::<model::VersionHistory<E>>(d.get_document("result").unwrap().clone())?) }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}

/// Retrieves the version history of a specific entity from the database.
///
/// # Type Parameters
/// - `E`: The entity type that implements the `Deserialize` trait, representing the deserialized
/// data for the version history.
///
/// # Arguments
/// - `id`: The `ObjectId` of the document whose version history is being queried.
/// - `application`: A string slice representing the application associated with the request.
/// - `database`: A string slice representing the database where the collection resides.
/// - `collection`: A string slice representing the collection in the database that contains the document.
///
/// # Returns
/// - `Ok(Vec<model::VersionHistory<E>>)` if the version history is successfully retrieved and parsed.
/// - `Err(Box<dyn Error>)` if there is an error in executing the request, deserializing the response,
/// or if the response contains an error or is invalid.
///
/// # Errors
/// This function returns an error if:
/// - The request execution fails.
/// - The database service response contains an error.
/// - The response cannot be deserialized properly.
/// - The response is invalid or contains unexpected data.
///
/// # Example
/// ```rust, no_run
/// use bson::oid::ObjectId;
/// use mongo_service::repository::{version_history_for, model::VersionHistory};
/// use serde::Deserialize;
///
/// #[derive(Debug, Deserialize)]
/// struct MyDocument {
///     field_a: String,
///     field_b: i32,
/// }
///
/// let object_id = ObjectId::parse_str("64e7eaa5fc13ae2a5b000001").unwrap();
/// let application = "my_app";
/// let database = "my_database";
/// let collection = "my_collection";
///
/// let result: Result<Vec<VersionHistory<MyDocument>>, Box<dyn std::error::Error>> =
///     version_history_for(object_id, application, database, collection);
///
/// match result {
///     Ok(history) => println!("Version history retrieved successfully: {:?}", history),
///     Err(e) => eprintln!("Failed to retrieve version history: {}", e),
/// }
/// ```
///
/// # Dependencies
/// - Requires the `mongodb` crate for BSON and ObjectId handling.
/// - Assumes `crate::client::cpp::execute` is the mechanism used to communicate with the database service.
/// - Depends on `model::VersionHistory<E>` to deserialize the version history data.
///
/// # Notes
/// - The function assumes specific keys in the database service response:
///   - `"results"`: Contains the version history data as an array.
///   - `"error"`: Contains error messages, if any.
/// - If no valid keys are found in the response, the function returns a generic error: `"Invalid response from mongo-service"`.
///
/// # Serialization/Deserialization
/// The function uses `serialize_from_document` to convert BSON documents into `VersionHistory<E>` objects.
pub fn version_history_for<E: for <'de> Deserialize<'de>>(id: ObjectId, application: &str,
  database: &str, collection: &str) -> Result<Vec<model::VersionHistory<E>>, Box<dyn Error>>
{
  let request = doc!{"application": application, "database": database, "collection": collection,
    "action": format!("{:?}", crate::Action::retrieve), "document": doc!{"entity._id": id},
    "options": doc!{"sort": doc!{"_id": 1}}};
  let bytes = request.to_vec()?;
  let response = crate::client::cpp::execute(bytes);
  if response.is_err() { return Err(response.unwrap_err().into()); }

  let d = Document::from_reader(response.unwrap().as_slice())?;
  if d.contains_key("results")
  {
    let a = d.get_array("results").unwrap();
    Ok(a.iter().map(|x| deserialize_from_document::<model::VersionHistory<E>>(x.as_document().unwrap().clone()).unwrap()).collect())
  }
  else if d.contains_key("error") { Err(d.get("error").unwrap().as_str().unwrap().into()) }
  else { Err("Invalid response from mongo-service".into()) }
}
