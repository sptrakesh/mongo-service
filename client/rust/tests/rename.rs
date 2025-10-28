use googletest::prelude::*;
use std::{thread, time::Duration};

mod common;

#[gtest]
#[test]
fn rename() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "oldname";
  let renamed = "newname";

  let res = rtests::create(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  let (id, vhid, vhd, vhc) = res?;

  let res = rtests::rename(database, collection, "test", true);
  expect_eq!(res.is_ok(), true);

  let res = rtests::rename(database, collection, renamed, false);
  expect_eq!(res.is_ok(), true);

  let res = common::id(database, renamed, &id);
  expect_eq!(res.is_ok(), true);

  let res = rtests::vid(vhd.as_str(), vhc.as_str(), &vhid, database, renamed, false);
  expect_eq!(res.is_ok(), true);

  let res = common::drop(database, renamed, true);
  verify_that!(res.is_ok(), eq(true))?;

  thread::sleep(Duration::from_millis(250));
  let res = rtests::vid(vhd.as_str(), vhc.as_str(), &vhid, database, renamed, true);
  expect_eq!(res.is_ok(), true);

  Ok(())
}

mod rtests
{
  use bson::{doc, oid::ObjectId};
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;

  pub fn create(database: &str, collection: &str) -> Result<(ObjectId, ObjectId, String, String)>
  {
    let oid = ObjectId::new();
    let doc = doc!{"_id": oid.clone(), "key": "value"};
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
          Ok((oid, d.get_object_id("_id")?, d.get_str("database")?.to_string(), d.get_str("collection")?.to_string()))
        }
      _ => panic!("Unexpected response")
    }
  }

  pub fn rename(database: &str, collection: &str, target: &str, error: bool) -> Result<()>
  {
    let request = Request::rename_collection(database.to_string(), collection.to_string(), doc!{"target": target});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          if error { panic!("Unexpected response"); }
          verify_that!(d.contains_key("database"), eq(true))?;
          verify_eq!(d.get("database").unwrap().as_str().unwrap(), database)?;
          verify_that!(d.contains_key("collection"), eq(true))?;
          verify_eq!(d.get("collection").unwrap().as_str().unwrap(), target)?;
        }
      Response::error(s) =>
        {
          if !error { panic!("Unexpected response"); }
          verify_that!(s.is_empty(), eq(false))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn vid(database: &str, collection: &str, id: &ObjectId, db: &str, coll: &str, error: bool) -> Result<()>
  {
    let request = Request::retrieve(database.to_string(), collection.to_string(),
      doc!{"_id": id, "database": db, "collection": coll});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          if error { panic!("Unexpected response"); }
          verify_that!(d.contains_key("_id"), eq(true))?;
          verify_that!(d.contains_key("entity"), eq(true))?;
          verify_that!(d.get_object_id("_id").unwrap(), eq(*id))?;

          let e = d.get_document("entity")?;
          verify_that!(e.contains_key("_id"), eq(true))?;
        }
      Response::error(s) =>
        {
          if !error { panic!("Unexpected response"); }
          verify_that!(s.is_empty(), eq(false))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}
