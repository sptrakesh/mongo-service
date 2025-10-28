use bson::{bson, oid::ObjectId};
use googletest::prelude::*;
use serde::{Deserialize, Serialize};

mod common;

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct Metadata
{
  pub year: String,
  pub month: String,
}

#[gtest]
#[test]
fn model() -> Result<()>
{
  common::init();

  let application = "rust-test";
  let database = "itest";
  let collection = "test";

  let p = common::Person { id: ObjectId::new(), name: "John Doe".to_string(), age: 43, phones: vec!["+44 1234567".to_string(), "+44 2345678".to_string()] };
  let now = chrono::Utc::now();
  let m = Metadata { year: now.format("%Y").to_string(), month: now.format("%m").to_string() };

  let req = mongo_service::repository::model::create::RequestBuilder::<common::Person, Metadata>::new(application, database, collection, p.clone()).with_metadata(m.clone()).build();
  let res = mongo_service::repository::create(req);
  verify_that!(res.is_ok(), eq(true))?;
  match res.unwrap()
  {
    mongo_service::repository::model::create::Response::WithHistory(h) => {
      expect_eq!(h.entity, p.id);
      expect_ne!(h.id, p.id);
    }
    _ => panic!("Unexpected response")
  }

  let res = mongo_service::repository::by_id::<common::Person>(p.id.clone(), application, database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  expect_eq!(res.unwrap(), p);

  let res = mongo_service::repository::property::<common::Person>("name", bson!(p.name.clone()), application, database, collection, 100, true);
  verify_that!(res.is_ok(), eq(true))?;
  let res = res.unwrap();
  verify_that!(res.len(), gt(0))?;
  for person in res.iter() { expect_eq!(person.name, p.name); }
  let mut found = false;
  for person in &res { if person.id == p.id { found = true; } }
  expect_eq!(found, true);

  let mut modified = p.clone();
  modified.name = "Jane Doe".to_string();
  let req = mongo_service::repository::model::update::RequestBuilder::<common::Person, Metadata>::new(application, database, collection, modified.clone()).with_metadata(m.clone()).build();
  let res = mongo_service::repository::update(req);
  verify_that!(res.is_ok(), eq(true))?;
  let res = res.unwrap();
  match res {
    mongo_service::repository::model::update::Response::WithHistory(h) =>
      {
        expect_eq!(h.document.id, p.id);
        expect_eq!(h.document.name, modified.name);
        expect_ne!(h.history.id, p.id);
      }
    _ => panic!("Unexpected response")
  }

  let res = mongo_service::repository::delete_by_id(p.id.clone(), application, database, collection);
  verify_that!(res.is_ok(), eq(true))?;
  let res = res.unwrap();
  expect_eq!(res.success.len(), 1);
  expect_eq!(res.failure.len(), 0);
  expect_eq!(res.history.len(), 1);

  let vhid = res.history[0].id;
  let vhdb = res.history[0].database.as_str();
  let vhc = res.history[0].collection.as_str();
  let res = mongo_service::repository::version_history_document::<common::Person>(vhid, application, vhdb, vhc);
  verify_that!(res.is_ok(), eq(true))?;
  let res = res.unwrap();
  expect_eq!(res.entity.id, modified.id);
  expect_eq!(res.entity.name, modified.name);
  expect_eq!(res.entity.age, modified.age);
  expect_eq!(res.entity.phones, modified.phones);

  let res = mongo_service::repository::version_history_for::<common::Person>(p.id.clone(), application, vhdb, vhc);
  verify_that!(res.is_ok(), eq(true))?;
  let res = res.unwrap();
  expect_eq!(res.len(), 3);
  for vhd in res.iter() { expect_eq!(vhd.entity.id, p.id); }

  Ok(())
}