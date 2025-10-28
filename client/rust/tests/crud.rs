use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn crud() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "test";

  let res = ctests::create(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  let (id, vhid, vhd, vhc) = res?;

  let res = common::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  let count = res?;
  expect_gt!(count, 0);

  let res = common::id(database, collection, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::property(database, collection, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::vid(vhd.as_str(), vhc.as_str(), &vhid, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::veid(vhd.as_str(), vhc.as_str(), &vhid, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::update(database, collection, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = ctests::update_nohistory(database, collection, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = common::delete(database, collection, &id);
  verify_that!(res.is_ok(), eq(true))?;

  let res = common::count(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_gt!(count, res?);

  Ok(())
}

mod ctests
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

  pub fn property(database: &str, collection: &str, id: &ObjectId) -> Result<()>
  {
    let request = Request::retrieve(database.to_string(), collection.to_string(), doc!{"key": "value"});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::results(d) =>
        {
          verify_ne!(d.len(), 0)?;
          let mut found = false;
          for a in d.iter()
          {
            let i = a.as_document().unwrap();
            if i.get_object_id("_id")? == *id { found = true; break; }
          }
          verify_that!(found, eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn vid(database: &str, collection: &str, hid: &ObjectId, id: &ObjectId) -> Result<()>
  {
    let request = Request::retrieve(database.to_string(), collection.to_string(), doc!{"_id": hid.clone()});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("_id"), eq(true))?;
          verify_that!(d.contains_key("entity"), eq(true))?;
          verify_that!(d.get_object_id("_id").unwrap(), eq(*hid))?;

          let e = d.get_document("entity")?;
          verify_that!(e.contains_key("_id"), eq(true))?;
          verify_that!(e.get_object_id("_id").unwrap(), eq(*id))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn veid(database: &str, collection: &str, hid: &ObjectId, id: &ObjectId) -> Result<()>
  {
    let request = Request::retrieve(database.to_string(), collection.to_string(), doc!{"entity._id": id.clone()});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::results(a) =>
        {
          verify_eq!(a.len(), 1)?;
          let d = a.get(0).unwrap().as_document().unwrap();
          verify_that!(d.contains_key("_id"), eq(true))?;
          verify_that!(d.contains_key("entity"), eq(true))?;
          verify_that!(d.get_object_id("_id").unwrap(), eq(*hid))?;

          let e = d.get_document("entity")?;
          verify_that!(e.contains_key("_id"), eq(true))?;
          verify_that!(e.get_object_id("_id").unwrap(), eq(*id))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn update(database: &str, collection: &str, id: &ObjectId) -> Result<()>
  {
    let request = Request::update(database.to_string(), collection.to_string(),
      doc!{"_id": id.clone(), "key": "value updated", "ival": 1234});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("document"), eq(true))?;
          verify_that!(d.contains_key("history"), eq(true))?;

          let ud = d.get_document("document")?;
          verify_that!(ud.contains_key("_id"), eq(true))?;
          verify_that!(ud.get_object_id("_id").unwrap(), eq(*id))?;
          verify_that!(ud.contains_key("key"), eq(true))?;
          verify_that!(ud.contains_key("ival"), eq(true))?;

          let ud = d.get_document("history")?;
          verify_that!(ud.contains_key("_id"), eq(true))?;
          verify_ne!(ud.get_object_id("_id").unwrap(), *id)?;
          verify_that!(ud.contains_key("entity"), eq(true))?;
          verify_that!(ud.get_object_id("entity").unwrap(), eq(*id))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }

  pub fn update_nohistory(database: &str, collection: &str, id: &ObjectId) -> Result<()>
  {
    let mut request = Request::update(database.to_string(), collection.to_string(),
      doc!{"_id": id.clone(), "key": "value updated", "ival": 1234, "dval": 123.45});
    request.skipVersion = true;
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("skipVersion"), eq(true))?;
          verify_that!(d.contains_key("document"), eq(false))?;
          verify_that!(d.contains_key("history"), eq(false))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}
