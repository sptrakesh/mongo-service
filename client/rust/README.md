# Rust Client
A simple Rust wrapper around the C++ API using [cxx.rs](https://cxx.rs/).  Similar to the C++ API, the client
exposes a low-level API, and a higher-level strongly typed API for working with domain models.
The higher-level API is more likely to be used in applications, while the low-level API is convenient 
for writing simple utilities.

See [build.rs](build.rs) for setting include search paths and library search paths and libraries.

## Use
Include the checked-out directory in your project workspace, or copy the sources into your
project and use as appropriate.

See the integration tests for usage examples.  The tests perform the same set of operations as the
C++ tests.

### High-Level API
<details>
<summary>Sample code</summary>

```rust
use bson::{bson, oid::ObjectId};
use serde::{Deserialize, Serialize};
use mongo_service::*;

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct Person
{
  pub phones: Vec<String>,
  pub name: String,
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId,
  pub age: i16
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct Metadata
{
  pub year: String,
  pub month: String,
}

fn model() -> Result<()>
{
  let mut logger = Logger::new("/tmp/", "mongo-service-rust");
  logger.level = LogLevel::DEBUG;
  init_logger(logger);

  init_service(Configuration::new("rust", "localhost", 2000));
  
  let application = "rust-test";
  let database = "itest";
  let collection = "test";

  let p = Person { id: ObjectId::new(), name: "John Doe".to_string(), age: 43, phones: vec!["+44 1234567".to_string(), "+44 2345678".to_string()] };
  let now = chrono::Utc::now();
  let m = Metadata { year: now.format("%Y").to_string(), month: now.format("%m").to_string() };

  let req = mongo_service::repository::model::create::RequestBuilder::<common::Person, Metadata>::new(application, database, collection, p.clone()).with_metadata(m.clone()).build();
  let res = mongo_service::repository::create(req);
  match res.unwrap()
  {
    mongo_service::repository::model::create::Response::WithHistory(h) => { log::info!("Created: {:?}", h); }
    _ => panic!("Unexpected response")
  }
  
  let res = mongo_service::repository::by_id::<Person>(p.id.clone(), application, database, collection);
  let res = mongo_service::repository::property::<Person>("name", bson!(p.name.clone()), application, database, collection, 100, true);
  
  let mut modified = p.clone();
  modified.name = "Jane Doe".to_string();
  let req = mongo_service::repository::model::update::RequestBuilder::<common::Person, Metadata>::new(application, database, collection, modified.clone()).with_metadata(m.clone()).build();
  let res = mongo_service::repository::update(req);
  let res = res.unwrap();
  match res {
    mongo_service::repository::model::update::Response::WithHistory(h) => { log::info!("Updated: {:?}", h); }
    _ => panic!("Unexpected response")
  }
  
  let res = mongo_service::repository::delete_by_id(p.id.clone(), application, database, collection);
}
```

</details>

### Low-Level API
<details>
<summary>Sample code</summary>

```rust
use mongo_service::*;
use bson::oid::ObjectId;
use googletest::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
struct Person
{
  phones: Vec<String>,
  name: String,
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  id: ObjectId,
  age: i16
}

#[test]
fn operations()
{
  let mut logger = Logger::new("/tmp/", "mongo-service-rust");
  logger.level = LogLevel::DEBUG;
  init_logger(logger);
  
  init_service(Configuration::new("rust", "localhost", 2000));
  
  let database = "itest";
  let collection = "test";
  let p = Person{id: ObjectId::new(), name: "John Doe".to_string(), age: 43, phones: vec!["+44 1234567".to_string(), "+44 2345678".to_string()]};

  let res = create(database, collection, &p);
  verify_that!(res.is_ok(), eq(true))?;

  let res = id(database, collection, &p);
  verify_that!(res.is_ok(), eq(true))?;

  let res = delete(database, collection, &p.id);
  verify_that!(res.is_ok(), eq(true))?;
}

fn create(database: &str, collection: &str, person: &Person) -> Result<()>
{
  let d = serialize_to_document(person)?;
  let request = Request::create(database.to_string(), collection.to_string(), d);
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        verify_that!(d.contains_key("_id"), eq(true))?;
        verify_that!(d.contains_key("entity"), eq(true))?;
        verify_that!(d.contains_key("database"), eq(true))?;
        verify_that!(d.contains_key("collection"), eq(true))?;
        verify_eq!(d.get_object_id("entity").unwrap(), person.id)?;
      }
    _ => panic!("Unexpected response")
  }

  Ok(())
}

fn id(database: &str, collection: &str, person: &Person) -> Result<()>
{
  let request = Request::retrieve(database.to_string(), collection.to_string(), doc!{"_id": person.id});
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        let p = deserialize_from_document::<Person>(d)?;
        verify_eq!(p, *person)?;
      }
    _ => panic!("Unexpected response")
  }

  Ok(())
}

fn delete(database: &str, collection: &str, id: &ObjectId) -> Result<()>
{
  let request = Request::delete(database.to_string(), collection.to_string(), doc!{"_id": id});
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        verify_that!(d.contains_key("success"), eq(true))?;
        verify_that!(d.contains_key("failure"), eq(true))?;
        verify_that!(d.contains_key("history"), eq(true))?;
        let a = d.get_array("success")?;
        verify_eq!(a.len(), 1)?;
        verify_eq!(a.get(0).unwrap().as_object_id().unwrap(), *id)?;
      }
    _ => panic!("Unexpected response")
  }

  Ok(())
}
```
</details>
