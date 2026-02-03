use bson::{DateTime, Document, oid::ObjectId};
use serde::{Deserialize, Serialize};

pub mod count;
pub mod create;
pub mod delete;
pub mod options;
pub mod retrieve;
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
  /// The entity that was created/updated/deleted.
  pub entity: E,
  /// Optional metadata that was specified when the entity was created/updated/deleted.
  pub metadata: Option<Document>,
  /// The database in which the entity was created/updated/deleted.
  pub database: String,
  /// The collection in which the entity was created/updated/deleted.
  pub collection: String,
  /// The action that was performed on the entity.
  pub action: String,
  /// The date and time at which the entity was created/updated/deleted.
  pub created: DateTime,
  /// The `_id` of the version history document that was created.
  #[serde(rename(serialize = "_id", deserialize = "_id"))]
  pub id: ObjectId
}
