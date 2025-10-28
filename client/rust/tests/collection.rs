use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn collection() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "timeseries";

  let res = ctests::create(database, collection);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::duplicate(database, collection);
  expect_that!(res.is_ok(), eq(true));

  let res = ctests::insert_auto_id(database, collection);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::insert(database, collection);
  verify_that!(res.is_ok(), eq(true))?;

  let res = common::drop(database, collection, false);
  verify_that!(res.is_ok(), eq(true))?;

  Ok(())
}

mod ctests
{
  use bson::{doc, datetime::DateTime, oid::ObjectId};
  use chrono::Utc;
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;

  pub fn create(database: &str, collection: &str) -> Result<()>
  {
    let doc = doc!{"timeseries": doc!{"timeField": "timestamp", "metaField": "data", "granularity": "hours"}};
    let request = Request::create_collection(database.to_string(), collection.to_string(), doc);
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("database"), eq(true))?;
          verify_that!(d.contains_key("collection"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn duplicate(database: &str, collection: &str) -> Result<()>
  {
    let doc = doc!{"timeseries": doc!{"timeField": "timestamp", "metaField": "data", "granularity": "hours"}};
    let request = Request::create_collection(database.to_string(), collection.to_string(), doc);
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::error(d) =>
        {
          verify_that!(d.is_empty(), eq(false))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn insert_auto_id(database: &str, collection: &str) -> Result<()>
  {
    let doc = doc!{"timestamp": DateTime::from_chrono(Utc::now()), "data": doc!{"sensor": "123"}, "value": 1.243};
    let request = Request::create_time_series(database.to_string(), collection.to_string(), doc);
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("_id"), eq(true))?;
          verify_that!(d.contains_key("database"), eq(true))?;
          verify_that!(d.contains_key("collection"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn insert(database: &str, collection: &str) -> Result<()>
  {
    let oid = ObjectId::new();
    let doc = doc!{"_id": oid, "timestamp": DateTime::from_chrono(Utc::now()), "data": doc!{"sensor": "123"}, "value": 1.243};
    let request = Request::create_time_series(database.to_string(), collection.to_string(), doc);
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("_id"), eq(true))?;
          expect_eq!(d.get_object_id("_id")?, oid);
          verify_that!(d.contains_key("database"), eq(true))?;
          verify_that!(d.contains_key("collection"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}