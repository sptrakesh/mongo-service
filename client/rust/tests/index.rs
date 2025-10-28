use bson::doc;
use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn index() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "test";
  let spec = doc!{"unused": 1};

  let res = itests::create(database, collection, &spec, true);
  expect_that!(res.is_ok(), eq(true));

  let res = itests::create(database, collection, &spec, false);
  expect_that!(res.is_ok(), eq(true));

  let res = itests::drop(database, collection, &spec);
  expect_that!(res.is_ok(), eq(true));

  let res = itests::unique(database, collection, &spec, true);
  expect_that!(res.is_ok(), eq(true));

  let res = itests::unique(database, collection, &spec, false);
  expect_that!(res.is_ok(), eq(true));

  let res = itests::drop_unique(database, collection);
  expect_that!(res.is_ok(), eq(true));

  Ok(())
}

mod itests
{
  use bson::{doc, Document};
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;

  pub fn create(database: &str, collection: &str, spec: &Document, flag: bool) -> Result<()>
  {
    let mut request = Request::index(database.to_string(), collection.to_string(), spec.clone());
    request.options = Some(doc!{"version": 2});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("name"), eq(flag))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn drop(database: &str, collection: &str, spec: &Document) -> Result<()>
  {
    let mut request = Request::drop_index(database.to_string(), collection.to_string(), doc!{"specification": spec});
    request.options = Some(doc!{"version": 2});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("dropIndex"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn unique(database: &str, collection: &str, spec: &Document, flag: bool) -> Result<()>
  {
    let mut request = Request::index(database.to_string(), collection.to_string(), spec.clone());
    request.options = Some(doc!{"name": "uniqueIndex", "unique": true, "expireAfterSeconds": 5});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("name"), eq(flag))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn drop_unique(database: &str, collection: &str) -> Result<()>
  {
    let mut request = Request::drop_index(database.to_string(), collection.to_string(), doc!{"name": "uniqueIndex"});
    request.options = Some(doc!{"version": 2});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("dropIndex"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}