use bson::{DateTime, Document, oid::ObjectId};
use serde::{Deserialize, Serialize};

pub mod create;
pub mod delete;
pub mod options;
pub mod update;

/// Structure returned from the service for a successful insert/update operation.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct History
{
  /// The database in which the version history document was created.
  pub database: String,
  /// The collection in which the version history document was created.
  pub collection: String,
  /// The `_id` of the entity that was created.
  pub entity: ObjectId,
  /// The `_id` of the version history document that was created.
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct VersionHistory<E>
{
  pub entity: E,
  pub metadata: Option<Document>,
  pub database: String,
  pub collection: String,
  pub action: String,
  pub created: DateTime,
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId
}
