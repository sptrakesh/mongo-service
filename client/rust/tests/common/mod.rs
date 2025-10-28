use bson::{doc, oid::ObjectId};
use mongo_service::{execute, init as minit, Configuration, Logger, Request, Response};
use googletest::prelude::*;
use serde::{Deserialize, Serialize};

#[allow(dead_code)]
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct Person
{
  pub phones: Vec<String>,
  pub name: String,
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId,
  pub age: i16
}

pub fn init()
{
  let logger = Logger::new("/tmp/", "mongo-service-rust");
  let conf = Configuration::new("rust", "localhost", 2000);
  minit(logger, conf);
}

#[allow(dead_code)]
pub fn id(database: &str, collection: &str, id: &ObjectId) -> Result<()>
{
  let request = Request::retrieve(database.to_string(), collection.to_string(), doc!{"_id": id});
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        verify_that!(d.contains_key("_id"), eq(true))?;
        verify_that!(d.get_object_id("_id").unwrap(), eq(*id))?;
      }
    _ => panic!("Unexpected response")
  }

  Ok(())
}

#[allow(dead_code)]
pub fn count(database: &str, collection: &str) -> Result<i64>
{
  let request = Request::count(database.to_string(), collection.to_string(), doc!{});
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        verify_that!(d.contains_key("count"), eq(true))?;
        Ok(d.get_i64("count")?)
      }
    _ => panic!("Unexpected response")
  }
}

#[allow(dead_code)]
pub fn delete(database: &str, collection: &str, id: &ObjectId) -> Result<()>
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

#[allow(dead_code)]
pub fn drop(database: &str, collection: &str, clear_history: bool) -> Result<()>
{
  let request = Request::drop_collection(database.to_string(), collection.to_string(), doc!{"clearVersionHistory": clear_history});
  let res = execute(request);
  verify_that!(res.is_ok(), eq(true))?;

  let res = res.unwrap();
  match res
  {
    Response::result(d) =>
      {
        verify_that!(d.contains_key("dropCollection"), eq(true))?;
      }
    _ => panic!("Unexpected response")
  }

  Ok(())
}
