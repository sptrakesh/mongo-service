#![allow(non_snake_case)]

use bson::Document;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Collation
{
  pub locale: Option<String>,
  pub caseFirst: Option<String>,
  pub alternate: Option<String>,
  pub maxVariable: Option<String>,
  pub strength: Option<u32>,
  pub caseLevel: Option<bool>,
  pub numericOrdering: Option<bool>,
  pub backwards: Option<bool>
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub enum WriteConcernAcknowlegeLevel
{
  Default, Majority, Tag, Unacknowledged, Acknowledged
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct WriteConcern
{
  pub acknowledgeLevel: Option<WriteConcernAcknowlegeLevel>,
  pub tag: Option<String>,
  pub majority: Option<u64>,
  pub timeout: Option<u64>,
  pub nodes: Option<u32>,
  pub journal: Option<bool>
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub enum ReadMode { Primary, PrimaryPreferred, Secondary, SecondaryPreferred, Nearest }

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct ReadPreference
{
  pub tags: Option<Document>,
  pub maxStaleness: Option<u64>,
  pub mode: ReadMode
}

impl ReadPreference
{
  pub fn new() -> Self
  {
    Self { tags: None, maxStaleness: None, mode: ReadMode::Primary }
  }
}
