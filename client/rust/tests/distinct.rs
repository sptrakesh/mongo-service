use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn distinct() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "test";

  let res = dtests::create(database, collection, "value1");
  verify_that!(res.is_ok(), eq(true))?;
  let id1 = res?;

  let res = dtests::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res?, 1);

  let res = dtests::create(database, collection, "value2");
  verify_that!(res.is_ok(), eq(true))?;
  let id2 = res?;

  let res = dtests::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res?, 2);

  let res = dtests::create(database, collection, "value2");
  verify_that!(res.is_ok(), eq(true))?;
  let id3 = res?;

  let res = dtests::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res?, 2);

  let res = dtests::create(database, collection, "value");
  verify_that!(res.is_ok(), eq(true))?;
  let id4 = res?;

  let res = dtests::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res?, 3);

  let res = dtests::filter(database, collection, &id4);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res?, 2);

  let res = dtests::invalid(database, collection);
  verify_that!(res.is_ok(), eq(true))?;

  let res = common::delete(database, collection, &id1);
  expect_that!(res.is_ok(), eq(true));

  let res = common::delete(database, collection, &id2);
  expect_that!(res.is_ok(), eq(true));

  let res = common::delete(database, collection, &id3);
  expect_that!(res.is_ok(), eq(true));

  let res = common::delete(database, collection, &id4);
  expect_that!(res.is_ok(), eq(true));

  Ok(())
}

mod dtests
{
  use bson::{doc, oid::ObjectId};
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;

  pub fn create(database: &str, collection: &str, value: &str) -> Result<ObjectId>
  {
    let oid = ObjectId::new();
    let doc = doc!{"_id": oid.clone(), "string": value};
    let request = Request::create(database.to_string(), collection.to_string(), doc);
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
          verify_eq!(d.get_object_id("entity").unwrap(), oid)?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(oid)
  }

  pub fn count(database: &str, collection: &str) -> Result<i64>
  {
    let request = Request::distinct(database.to_string(), collection.to_string(), doc!{"field": "string"});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::results(a) =>
        {
          verify_eq!(a.len(), 1)?;
          let d = a.get(0).unwrap().as_document().unwrap();
          verify_that!(d.contains_key("values"), eq(true))?;
          Ok(d.get_array("values").unwrap().len() as i64)
        }
      _ => panic!("Unexpected response")
    }
  }

  pub fn filter(database: &str, collection: &str, id: &ObjectId) -> Result<i64>
  {
    let doc = doc!{"field": "string", "filter": doc!{"_id": doc!{"$ne": id}}};
    let request = Request::distinct(database.to_string(), collection.to_string(), doc);
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::results(a) =>
        {
          verify_eq!(a.len(), 1)?;
          let d = a.get(0).unwrap().as_document().unwrap();
          verify_that!(d.contains_key("values"), eq(true))?;
          Ok(d.get_array("values").unwrap().len() as i64)
        }
      _ => panic!("Unexpected response")
    }
  }

  pub fn invalid(database: &str, collection: &str) -> Result<()>
  {
    let request = Request::distinct(database.to_string(), collection.to_string(), doc!{"field": "invalid"});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::results(a) =>
        {
          verify_eq!(a.len(), 1)?;
          let d = a.get(0).unwrap().as_document().unwrap();
          verify_that!(d.contains_key("values"), eq(true))?;
          let d = d.get_array("values").unwrap();
          verify_eq!(d.len(), 0)?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}