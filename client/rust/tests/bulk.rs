use googletest::prelude::*;

mod common;

#[gtest]
#[test]
fn bulk() -> Result<()>
{
  common::init();

  let database = "itest";
  let collection = "test";

  let res = btests::create(database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  let ids = res?;

  let res = common::count(database, collection);
  expect_eq!(res.is_ok(), true);
  let count = res?;
  expect_ge!(count, ids.len() as i64);

  let res = btests::delete(database, collection, &ids);
  expect_eq!(res.is_ok(), true);

  let res = common::count(database, collection);
  expect_eq!(res.is_ok(), true);
  let ncount = res?;
  expect_eq!(ncount, count - ids.len() as i64);

  Ok(())
}

mod btests
{
  use bson::{bson, doc, oid::ObjectId};
  use mongo_service::{execute, Request, Response};
  use googletest::prelude::*;

  pub fn create(database: &str, collection: &str) -> Result<Vec<ObjectId>>
  {
    let v = vec![ObjectId::new(), ObjectId::new()];
    let arr = bson!([doc!{"_id": v[0], "key": "value1"}, doc!{"_id": v[1], "key": "value2"}]);
    let request = Request::bulk(database.to_string(), collection.to_string(), doc!{"insert": arr});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("create"), eq(true))?;
          verify_that!(d.contains_key("history"), eq(true))?;
          verify_that!(d.contains_key("remove"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(v)
  }

  pub fn delete(database: &str, collection: &str, ids: &Vec<ObjectId>) -> Result<()>
  {
    let arr = bson!([doc!{"_id": ids[0]}, doc!{"_id": ids[1]}]);
    let request = Request::bulk(database.to_string(), collection.to_string(), doc!{"remove": arr});
    let res = execute(request);
    verify_that!(res.is_ok(), eq(true))?;

    let res = res.unwrap();
    match res
    {
      Response::result(d) =>
        {
          verify_that!(d.contains_key("create"), eq(true))?;
          verify_that!(d.contains_key("history"), eq(true))?;
          verify_that!(d.contains_key("remove"), eq(true))?;
        }
      _ => panic!("Unexpected response")
    }

    Ok(())
  }
}
