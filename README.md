# Mongo Service
A TCP service for routing all requests to **MongoDB** via a centralised service.
A few common features made available by this service are:
* All documents are versioned in a *version history* collection.
* All operations are timed and the metrics stored in a *metrics* collection.

## Version History
All documents stored in the database will automatically be *versioned* on save.
Deleting a document will move the current document into the *version history*
database *collection*.  This makes it possible to retrieve previous *versions*
of a document as needed, as well as restore a document (regardless of whether
it has been *deleted* or not).

## Protocol
All interactions are via *BSON* documents sent to the service.  Each request must
conform to the following document model:
* `action` - The type of database action being performed.  One of `create|retrieve|update|delete`.
* `database` - The Mongo database the action is to be performed against.
* `collection` - The Mongo collection the action is to be performed against.
* `document` - The document payload to associate with the database operation.
  For `create` and `update` this is assumed to be the documet that is being saved.
  For `retrieve` this is the *query* to execute.  For `delete` this is a simple
  `document` with an `_id` field.
* `options` - The options to associate with the Mongo request.  These correspond
  to the appropriate options as used by the Mongo driver.
  
### Limitation
At present only documents with **BSON ObjectId** `_id` is supported.

