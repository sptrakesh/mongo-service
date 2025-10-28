use bson::oid::ObjectId;
use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn model() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "test";

  let p = common::Person{id: ObjectId::new(), name: "John Doe".to_string(), age: 43, phones: vec!["+44 1234567".to_string(), "+44 2345678".to_string()]};

  let res = mtests::create(database, collection, &p);
  verify_that!(res.is_ok(), eq(true))?;

  let res = mtests::id(database, collection, &p);
  verify_that!(res.is_ok(), eq(true))?;

  let res = common::delete(database, collection, &p.id);
  verify_that!(res.is_ok(), eq(true))?;

  Ok(())
}

mod mtests
{
  use bson::{doc, deserialize_from_document, serialize_to_document};
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;
  use crate::common::Person;

  pub fn create(database: &str, collection: &str, person: &Person) -> Result<()>
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

  pub fn id(database: &str, collection: &str, person: &Person) -> Result<()>
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
}