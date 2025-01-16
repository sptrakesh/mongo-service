# Mongo Service

* [Command Line Options](#command-line-options)
* [Version History](#version-history)
* [Protocol](#protocol)
  * [Document Payload](#document-payload)
    * [Create](#create)
    * [Retrieve](#retrieve)
    * [Count](#count)
    * [Distinct](#distinct)
    * [Update](#update)
    * [Delete](#delete)
    * [Index](#index)
    * [Drop Index](#drop-index)
    * [Bulk Write](#bulk-write)
    * [Aggregation Pipeline](#aggregation-pipeline)
    * [Transaction](#transaction)
    * [Create Timeseries](#create-timeseries)
    * [Create Collection](#create-collection)
    * [Rename Collection](#rename-collection)
  * [Document Response](#document-response)
  * [Limitation](#limitation)
* [Metrics](#metrics)
  * [MongoDB](#mongodb) 
  * [ILP](#ilp)
* [Serialisation](#serialisation)
  * [BSON](#bson)
  * [JSON](#json)
* [Testing](#testing)
  * [Connection Pool](#connection-pool)
  * [Performance Test](#performance-test)
* [Build](#build)
  * [Windows](#windows)
    * [Limitations](#limitations)
* [Clients](#clients)
  * [API](#api-usage)
* [Acknowledgements](#acknowledgements)
    
A TCP service for routing all requests to **MongoDB** via a centralised service.
A few common features made available by this service are:
* All documents are versioned in a *version history* collection.
* All operations are timed and the metrics stored in a *metrics* collection.

**Note:** A C++17 compiler with *coroutines-ts*, or a C++20 compiler with
*coroutines* support is required to build this project.

## Command Line Options
The service can be configured via the following command line options:

* `port` - Specify the port the service is to bind to via the `-p` or `--port`
option.  Default is `2020`.
* `threads` - The number of **Boost ASIO IO Context** threads to use via the
`-n` or `--threads` option.  Default is the value returned by `std::thread::hardware_concurrency`.
* `mongoUri` - The full **MongoDB** *connection uri* to use.  This should include
the *user credentials* as well.  Specify via the `-m` or `--mongo-uri` option.
**Mandatory** option to start the service.
* `versionHistoryDatabase` - The data to use to store *version history* documents.
Specify via the `-d` or `--version-history-database` option.  Default `versionHistory`.
* `versionHistoryCollection` - The collection to store version history documents
in.  Specify via the `-c` or `--version-history-collection` option.  Default `entities`.
* `metricsDatabase` - The database in which to store request processing metrics
to.  Specify via the `-s` or `--metric-database` option.  Default `versionHistory`.
* `metricsCollection` - The collection in which to store the metric documents.
Specify via the `-t` or `--metric-collection` option.  Default `metrics`.
* `metricBatchSize` - The number of metrics to accumulate before saving to the desired store.
  Specify via the `-w` or `--metric-batch-size` option.  Default `100`.
* `ilpServer` - The host name for the time series database that supports the ILP.
  Specify via the `-i` or `--ilp-server` option.
* `ilpPort` - The port for the time series database that supports the ILP.
  Specify via the `-x` or `--ilp-port` option.
* `ilpMeasurement` - The series/measurement name for metrics.
  Specify via the `-y` or `--ilp-series-name` option.  Default `metrics`.
* `logLevel` - The logging level for the service.  Specify via `-l` or `--log-level`
option.  Default `info`.  Allowed values - `debug, info, warn, critical`.
* `logAsync` - Use asynchronous logger for the service (non-guaranteed and may
  lose some logs).  Specify via `-z` or `--log-async` option.  Default `true`.
  Allowed values - `true, false`.
* `console` - Whether log messages are to be echoed to the `console` as well.
Specify `true` via the `-c` or `--console` option.  Default `false`.
* `dir` - Specify the output directory under which log files are to be stored
via the `-o` or `--dir` option.  Note that a trailing slash `/` is **mandatory**.
Default `logs/`.

## Version History
All documents stored in the database will automatically be *versioned* on save.
Deleting a document will move the current document into the *version history*
database *collection*.  This makes it possible to retrieve previous *versions*
of a document as needed, as well as restore a document (regardless of whether
it has been *deleted* or not).

## Protocol
All interactions are via *BSON* documents sent to the service.  Each request must
conform to the following document model:
* `action (string)` - The type of database action being performed.  One of 
  `create|retrieve|update|delete|count|distinct|index|dropCollection|dropIndex|bulk|pipeline|transaction|createTimeseries|createCollection|renameCollection`.
* `database (string)` - The Mongo database the action is to be performed against.
    - Not needed for `transaction` action.
* `collection (string)` - The Mongo collection the action is to be performed against.
    - Not needed for `transaction` action.
* `document (document)` - The document payload to associate with the database operation.
  For `create` and `update` this is assumed to be the document that is being saved.
  For `retrieve` or `count` this is the *query* to execute.  For `delete` this
  is a simple `document` with an `_id` field.
* `options (document)` - The options to associate with the Mongo request.  These correspond
  to the appropriate options as used by the Mongo driver.
* `metadata (document)` - Optional *metadata* to attach to the version history document that
  is created (not relevant for `retrieve` obviously).  This typically will include
  information about the user performing the action, any other information as
  relevant in the system that uses this service.
* `application` - Optional name of the *application* accessing the service.
  Helps to retrieve database metrics for a specific *application*.
* `correlationId (string)` - Optional *correlation id* to associate with the metric record
  created by this action.  This is useful for tracing logs originating from a single
  operation/request within the overall system.  This value is stored as a *string*
  value in the *metric* document to allow for sensible data types to used as the
  *correlation id*.
* `skipVersion (bool)` - Optional `bool` value to indicate not to create a *version history*
  document for this `action`.  Useful when creating non-critical data such as
  logs.
* `skipMetric (bool)` - Optional `bool` value to indicate not to create a *metric*
  document for this `action`.  Useful when calls are made a part of a monitoring framework, and volume of metrics
  generated overwhelms storage requirements.

### Document Payload
The document payload contains all the information necessary to execute the
specified `action` in the request.

#### Create
The `document` as specified will be inserted into the specified `database` and
`collection`.  The **BSON ObjectId** property/field (`_id`) must be included in
the document.  The `_id` is needed within the version history document associated
with the *create* action.  If using *unacknowledged* writes, the auto-generated
`_id` is not available, and hence we would need to create a new *insert* document
with the `_id` set within the service.  This involves a wasteful copy of data,
and hence we enforce the requirement on client specified `_id` value.

Sample request payload (see [create.hpp](src/api/model/request/create.hpp)):
```json
{
  "action": "create",
  "database": "<database name>",
  "collection": "<collection name>",
  "document": {
    "_id": {
      "$oid": "5f35e5e1e799c52186039122"
    },
    "intValue": 123,
    "floatValue": 123.0,
    "boolValue": true,
    "stringValue": "abc123",
    "nested": {"key":  "value"}
  }
}
```

Sample response payload when version history document is created (default option) (see `Create` struct in
[create.hpp](src/api/model/response/create.hpp)):
```json
{
  "_id": {
    "$oid": "5f35e5e19e48c37186539141"
  },
  "database": "versionHistory",
  "collection": "entities",
  "entity": {
    "$oid": "5f35e5e1e799c52186039122"
  }
}
```

**Note:** The `_id` in the response is the object id of the version history document that was created.

Sample response payload when version history document is not created:
```json
{
  "_id": {
    "$oid": "5f35e5e19e48c37186539141"
  },
  "skipVersion": true
}
```

**Note:** The `_id` in the response is the object id for the document as specified in the input payload.

##### Options
The following options are supported for the `create` action (see `Create` struct in [insert.hpp](src/api/options/insert.hpp)):
* `bypassValidation` - *boolean*.  Whether or not to bypass document validation
* `ordered` - *boolean*.  Whether or not the `insert_many` will be ordered
* `writeConcern` - *document*.  The write concern for the operation.
  * `journal` - *boolean*.  If `true` confirms that the database has written the data to the on-disk journal before
    reporting a write operations was successful. This ensures that data is not lost if the database shuts down unexpectedly. 
  * `nodes` - *integer*.  Sets the number of nodes that are required to acknowledge the write before the operation is
    considered successful. Write operations will block until they have been replicated to the specified number of
    servers in a replica set.
  * `acknowledgeLevel` - *integer*.  Sets the [acknowledgement](https://www.mongodb.com/docs/manual/reference/write-concern/#w-option) level for the write operation.
    * `0` - Represent the implicit default write concern.
    * `1` - Represent write concern with `w: "majority"`.
    * `2` - Represent write concern with `w: <custom write concern name>`.
    * `3` - Represent write concern for un-acknowledged writes.
    * `4` - Represent write concern for acknowledged writes.
  * `majority` - *integer*.  The amount of time (milliseconds) to wait before the write operation times out if it 
    cannot reach the majority of nodes in the replica set. If the value is zero, then no timeout is set.
  * `tag` - *string*.  Sets the name representing the server-side `getLastErrorMode` entry containing the list of 
    nodes that must acknowledge a write operation before it is considered a success.
  * `timeout` - *integer*.  Sets an upper bound on the time (milliseconds) a write concern can take to be satisfied.
    If the write concern cannot be satisfied within the timeout, the operation is considered a failure.

#### Retrieve
Retrieve obviously does not have any interaction with the version history system
(unless you are retrieving versions).  We provide this since one of the other
purposes behind this service is to route/proxy all datastore interactions via
this service.

Sample retrieve payload (see [retrieve.hpp](src/api/model/request/retrieve.hpp)):
```json
{
  "action": "retrieve",
  "database": "itest",
  "collection": "test",
  "document": {
    "_id": {
      "$oid": "5f35e6d8c7e3a976365b3751"
    }
  }
}
```

Sample response (see `Retrieve` struct in [retrieve.hpp](src/api/model/response/retrieve.hpp)):
```json
{
  "result": {
    "_id": {
      "$oid": "5f35e5e1e799c52186039122"
    },
    "intValue": 123,
    "floatValue": 123.0,
    "boolValue": true,
    "stringValue": "abc123",
    "nested": {"key":  "value"}
  }
}
```

##### Options
The following options are supported for the `retrieve` action (See `Find` struct in [find.hpp](src/api/options/find.hpp)):

* `partialResults` - *boolean*.  Whether to allow partial results from database if some shards are down (instead of throwing an error).
* `batchSize` - *integer*.  The number of documents to return per batch.
* `collation` - *document*.  Sets the collation for this operation.
* `comment` - *string*.  Attaches a comment to the query.
* `commentOption` - *document*.  Set the value of the comment option.
* `hint` - *document*.  Sets the index to use for this operation.
* `let` - *document*.  Set the value of the let option.
* `limit` - *integer*.  The maximum number of documents to return.
* `max` - *document*.  Gets the current exclusive upper bound for a specific index.
* `maxTime` - *integer*.  The maximum amount of time for this operation to run (server-side) in milliseconds.
* `min` - *document*.  Gets the current exclusive lower bound for a specific index.
* `projection` - *document*.  Sets a projection which limits the returned fields for all matching documents.
* `readPreference` - *document*.  The read preference for the operation.
  * `tags` - *document*.  Sets the tag set list.
  * `hedge` - *document*.  Sets the hedge document to be used for the read preference. Sharded clusters running MongoDB 4.4 or later can dispatch read operations in parallel, returning the result from the fastest host and cancelling the unfinished operations.
  * `maxStaleness` - *integer*.  Sets the max staleness (seconds) setting. Secondary servers with an estimated lag greater than this value will be excluded from selection under modes that allow secondaries.
  * `mode` - *integer*.  Sets the read preference mode.  Valid values are:
    * `0` - Only read from a primary node.
    * `1` - Prefer to read from a primary node.
    * `2` - Only read from secondary nodes.
    * `3` - Prefer to read from secondary nodes.
    * `4` - Read from the node with the lowest latency irrespective of state.
* `returnKey` - *boolean*.  Whether to return the index keys associated with the query results, instead of the actual query results themselves.
* `showRecordId` - *boolean*.  Whether to include the record identifier for each document in the query results.
* `skip` - *integer*.  The number of documents to skip before returning results.
* `sort` - *document*.  The order in which to return matching documents.

#### Count
Count the number of documents matching the specified query document.
The query document can be empty to get the count of all documents in the specified
collection.

Sample count payload (see `Count` struct in [count.hpp](src/api/model/request/count.hpp)):
```json
{
  "action": "count",
  "database": "itest",
  "collection": "test",
  "document": {}
}
```

Sample response (see `Count` struct in [count.hpp](src/api/model/response/count.hpp)):
```json
{ "count" : 11350 }
```

##### Options
The following options are supported for the `count` action (see `Count` struct in [count.hpp](src/api/options/count.hpp)):

* `collation` - *document*.  Sets the collation for this operation.
* `hint` - *document*.  Sets the index to use for this operation.
* `limit` - *integer*.  The maximum number of documents to count.
* `maxTime` - *integer*.  The maximum amount of time for this operation to run (server-side) in milliseconds.
* `skip` - *integer*.  The number of documents to skip before counting documents.
* `readPreference` - *document*.  The read preference for the operation.
  * `tags` - *document*.  Sets the tag set list.
  * `hedge` - *document*.  Sets the hedge document to be used for the read preference. Sharded clusters running MongoDB 4.4 or later can dispatch read operations in parallel, returning the result from the fastest host and cancelling the unfinished operations.
  * `maxStaleness` - *integer*.  Sets the max staleness (seconds) setting. Secondary servers with an estimated lag greater than this value will be excluded from selection under modes that allow secondaries.
  * `mode` - *integer*.  Sets the read preference mode.  Valid values are:
    * `0` - Only read from a primary node.
    * `1` - Prefer to read from a primary node.
    * `2` - Only read from secondary nodes.
    * `3` - Prefer to read from secondary nodes.
    * `4` - Read from the node with the lowest latency irrespective of state.

#### Distinct
Retrieve distinct values for the specified field in documents in a collection.  The payload document *must* contain
the `field` for which distinct values are to be retrieved.  An optional `filter` field can be used to specify the 
filter query to use when retrieving distinct values.

Sample distinct payload (see `Distinct` struct in [distinct.hpp](src/api/model/request/distinct.hpp)):
```json
{
  "action": "distinct",
  "database": "itest",
  "collection": "test",
  "document": {
    "field": "myProp",
    "filter": {
      "deleted": {"$ne": true}
    }
  }
}
```

Sample response (see `Distinct` struct in [distinct.hpp](src/api/model/response/distinct.hpp)):
```json
{ "results" : [ { "values" : [ "value", "value1", "value2" ], "ok" : 1.0 } ] }
```

**Note:**
An array of `results` is returned since the mongo C++ API implementation for the `distinct`
command returns a *cursor*.  The documentation indicates only a single document is returned,
which implies we could return a `result` with just the document.  We chose not to make that
assumption in case the interface changes down the road (again because of the cursor returned by
the driver).

An empty `values` array is returned if the specified `field` does not exist in the documents
in the specified collection.

##### Options
The following options are supported for the `distinct` action (see `Distinct` struct in [distinct.hpp](src/api/options/distinct.hpp)):

* `collation` - *document*.  Sets the collation for this operation.
* `maxTime` - *integer*.  The maximum amount of time for this operation to run (server-side) in milliseconds.
* `readPreference` - *document*.  The read preference for the operation.
  * `tags` - *document*.  Sets the tag set list.
  * `hedge` - *document*.  Sets the hedge document to be used for the read preference. Sharded clusters running MongoDB 4.4 or later can dispatch read operations in parallel, returning the result from the fastest host and cancelling the unfinished operations.
  * `maxStaleness` - *integer*.  Sets the max staleness (seconds) setting. Secondary servers with an estimated lag greater than this value will be excluded from selection under modes that allow secondaries.
  * `mode` - *integer*.  Sets the read preference mode.  Valid values are:
    * `0` - Only read from a primary node.
    * `1` - Prefer to read from a primary node.
    * `2` - Only read from secondary nodes.
    * `3` - Prefer to read from secondary nodes.
    * `4` - Read from the node with the lowest latency irrespective of state.

#### Update
Update is the most complex scenario.  The service supports the two main update
*modes* supported by Mongo:
* **update** - The data specified in the `document` sub-document is merged into
 the existing document(s).
* **replace** - The data specified in the `document` is used to replace an existing
 document.
 
Updates are possible either by an explicit `_id` field in the input `document`,
or via a `filter` sub-document that expresses the query used to identify the
candidate document(s) to update.

If both `_id` and `filter` are omitted an error document is returned.

The returned BSON document depends on whether a single-document or multi-document
update request was made:
* *Single-Document* - For single document updates the full updated stored document
 (`document`) and basic information about the associated version history document
 (`history`) are returned.
* *Multi-Document* - For multi-document updates, an array of BSON object ids for
successful updates (`success`), failed updates (`failure`), and the basic
information about the version history documents (`history`).
 
##### Update By Id
The simple and direct update use case.  If the `document` has an `_id` property,
the remaining properties are merged into the stored document.  A version history
document with the resulting stored document is also created.

Sample update request by `_id` (see `MergeForId` struct in [update.hpp](src/api/model/request/update.hpp)):
```json
{
  "action": "update",
  "database": "itest",
  "collection": "test",
  "document": {
    "key1": "value1",
    "_id": {
      "$oid": "5f35e887bb516401e02b4701"
    }
  }
}
```

Sample response (see `Update` struct in [update.hpp](src/api/model/response/update.hpp)):
```json
{
  "document": {
    "_id": {
      "$oid": "5f35e887bb516401e02b4701"
    },
    "key": "value",
    "key1": "value1"
  },
  "history": {
    "_id": {
      "$oid": "5f35e887e799c5218603915b"
    },
    "database": "itest",
    "collection": "test",
    "entity": {
      "$oid": "5f35e887bb516401e02b4701"
    }
  }
}
```

##### Update without version history
Sample request payload:
```json
{
  "action": "update",
  "database": "itest",
  "collection": "test",
  "skipVersion": true,
  "document": {
    "key1": "value1",
    "_id": {
      "$oid": "5f35e932d3698352cb3bd2d1"
    }
  }
}
```

In this case, only a placeholder response is returned as follows:
```json
{ "skipVersion" : true }
```

##### Replace Document
If the `document` has a `replace` sub-document, then the existing document as
specified by the `filter` query will be replaced.  MongoDB will return as error
if an attempt is made to replace multiple documents (the query filter **must** return
a single document).  A version history document is created with the replaced
document.

The following sample shows an example of performing a `replace` action (see struct `Replace` in [update.hpp](src/api/model/request/update.hpp)).
```json
{
  "action": "update",
  "database": "itest",
  "collection": "test",
  "document": {
    "filter": {
      "_id": { "$oid": "5f3bc9e2502422053e08f9f1" }
    },
    "replace": {
      "_id": { "$oid": "5f3bc9e2502422053e08f9f1" },
      "key": "value",
      "key1": "value1"
    }
  },
  "options": { "upsert": true },
  "application": "version-history-api",
  "metadata": {
    "revertedFrom": { "$oid": "5f3bc9e29ba4f45f810edf29" }
  }
}
```

Response data is identical to [Update by Id](#update-by-id)

##### Update Document
If the `document` has a `update` sub-document, then existing document(s) are
updated with the information contained in it.  This is a *merge* operation where
only the fields specified in the `update` are set on the candidate document(s).
A version history document is created for each updated document.

If the input `filter` sub-document has an `_id` property, and is of type BSON
*ObjectId*, then a single document update is made.

###### Update with unset
As a simplification, it is possible to omit the `$set` operator, if used in conjunction with a `$unset` operator.  All
top level properties other than `_id` and `$unset` are implicitly added to a `$set` document in the actual update document
sent to MongoDB.

Sample request payload with explicit `$set` (see `Update` struct in [update.hpp](src/api/model/request/update.hpp)):
```json
{
  "action": "update",
  "database": "itest",
  "collection": "test",
  "document": {
    "filter": {
      "_id": {"$oid": "6435a62316d2310e800e4bf2"}
    },
    "update": {
      "$unset": {"obsoleteProperty": 1},
      "$set": {
        "metadata.modified": {"$date": 1681237539583},
        "metadata.user._id": {"$oid": "5f70ee572fc09200086c8f24"},
        "metadata.user.username": "mqtt"
      }
    }
  }
}
```

Sample request payload without `$set`:
```json
{
  "action": "update",
  "database": "itest",
  "collection": "test",
  "document": {
    "filter": {
      "_id": {"$oid": "6435a62316d2310e800e4bf2"}
    },
    "update": {
      "$unset": {"obsoleteProperty": 1},
      "metadata.modified": {"$date": 1681237539583},
      "metadata.user._id": {"$oid": "5f70ee572fc09200086c8f24"},
      "metadata.user.username": "user"
    }
  }
}
```

Response data has the following structure (see `UpdateMany` struct in [update.hpp](src/api/model/response/update.hpp))
```json
{
  "success": [{"$oid":  "6435a62316d2310e800e4bf2"}],
  "failure": [{"$oid":  "6435a62316d2310e800e4bf2"}],
  "history": [
    {
      "_id": {"$oid": "5f35e887e799c5218603915b"},
      "database": "itest",
      "collection": "test",
      "entity": {"$oid": "6435a62316d2310e800e4bf2"}
    }
  ]
}
```

##### Options
The following options are supported for the `update` action (see `Update` struct in [update.hpp](src/api/options/update.hpp)):
* `bypassValidation` - *boolean*.  Whether or not to bypass document validation
* `collation` - *document*.  Sets the collation for this operation.
* `upsert` - *boolean*.  By default, if no document matches the filter, the update operation does nothing. However, by 
  specifying upsert as `true`, this operation either updates matching documents or *inserts* a new document using the
  update specification if no matching document exists.
* `writeConcern` - *document*.  The write concern for the operation.
  * `journal` - *boolean*.  If `true` confirms that the database has written the data to the on-disk journal before
    reporting a write operations was successful. This ensures that data is not lost if the database shuts down unexpectedly.
  * `nodes` - *integer*.  Sets the number of nodes that are required to acknowledge the write before the operation is
    considered successful. Write operations will block until they have been replicated to the specified number of
    servers in a replica set.
  * `acknowledgeLevel` - *integer*.  Sets the [acknowledgement](https://www.mongodb.com/docs/manual/reference/write-concern/#w-option) level for the write operation.
    * `0` - Represent the implicit default write concern.
    * `1` - Represent write concern with `w: "majority"`.
    * `2` - Represent write concern with `w: <custom write concern name>`.
    * `3` - Represent write concern for un-acknowledged writes.
    * `4` - Represent write concern for acknowledged writes.
  * `majority` - *integer*.  The amount of time (milliseconds) to wait before the write operation times out if it
    cannot reach the majority of nodes in the replica set. If the value is zero, then no timeout is set.
  * `tag` - *string*.  Sets the name representing the server-side `getLastErrorMode` entry containing the list of
    nodes that must acknowledge a write operation before it is considered a success.
  * `timeout` - *integer*.  Sets an upper bound on the time (milliseconds) a write concern can take to be satisfied.
    If the write concern cannot be satisfied within the timeout, the operation is considered a failure.
* `arrayFilters` - *array*.  Array representing filters determining which array elements to modify.
 
#### Delete
The `document` represents the *query* to execute to find the candidate documents
to delete from the `database`:`collection`.  The query is executed to retrieve
the candidate documents, and the documents removed from the specified
`database:collection`.  The retrieved documents are then written to the version
history database.

Sample delete request (see `Delete` struct in [delete.hpp](src/api/model/request/delete.hpp)):
```json
{
  "action": "delete",
  "database": "itest",
  "collection": "test",
  "document": {
    "_id": { "$oid": "5f35ea61aa4ef01184492d71" }
  }
}
```

Sample delete response (see `Delete` struct in [delete.hpp](src/api/model/response/delete.hpp)):
```json
{
  "success": [{
    "$oid": "5f35ea61aa4ef01184492d71"
  }],
  "failure": [],
  "history": [{
    "_id": {
      "$oid": "5f35ea61e799c521860391a9"
    },
    "database": "versionHistory",
    "collection": "entities",
    "entity": {
      "$oid": "5f35ea61aa4ef01184492d71"
    }
  }]
}
```

##### Options
The following options are supported for the `delete` action (see `Delete` struct in [delete.hpp](src/api/options/delete.hpp)):
* `collation` - *document*.  Sets the collation for this operation.
* `writeConcern` - *document*.  The write concern for the operation.
  * `journal` - *boolean*.  If `true` confirms that the database has written the data to the on-disk journal before
    reporting a write operations was successful. This ensures that data is not lost if the database shuts down unexpectedly.
  * `nodes` - *integer*.  Sets the number of nodes that are required to acknowledge the write before the operation is
    considered successful. Write operations will block until they have been replicated to the specified number of
    servers in a replica set.
  * `acknowledgeLevel` - *integer*.  Sets the [acknowledgement](https://www.mongodb.com/docs/manual/reference/write-concern/#w-option) level for the write operation.
    * `0` - Represent the implicit default write concern.
    * `1` - Represent write concern with `w: "majority"`.
    * `2` - Represent write concern with `w: <custom write concern name>`.
    * `3` - Represent write concern for un-acknowledged writes.
    * `4` - Represent write concern for acknowledged writes.
  * `majority` - *integer*.  The amount of time (milliseconds) to wait before the write operation times out if it
    cannot reach the majority of nodes in the replica set. If the value is zero, then no timeout is set.
  * `tag` - *string*.  Sets the name representing the server-side `getLastErrorMode` entry containing the list of
    nodes that must acknowledge a write operation before it is considered a success.
  * `timeout` - *integer*.  Sets an upper bound on the time (milliseconds) a write concern can take to be satisfied.
    If the write concern cannot be satisfied within the timeout, the operation is considered a failure.
* `hint` - *document*.  Sets the index to use for this operation.
* `let` - *document*.  Set the value of the let option.

#### Drop Collection
Drop the specified collection and all its containing documents.  Specify an
empty `document` in the payload to satisfy payload requirements.  If you wish
to also remove all version history documents for the dropped collection, specify
`clearVersionHistory` `true` in the `document` (revision history documents
will be removed *asynchronously*). Specify the *write concern* settings in the
optional `options` sub-document.

Sample drop payload specifying removal of all associated revision history documents (see `DropCollection` struct 
in [dropcollection.hpp](src/api/model/request/dropcollection.hpp)):
```json
{
  "action": "dropCollection",
  "database": "itest",
  "collection": "test",
  "document": {"clearVersionHistory": true}
}
```

Sample response (see `DropCollection` struct in [collection.hpp](src/api/model/response/collection.hpp)):
```json
{ "dropCollection" : true }
```

##### Options
The following options are supported for the `dropCollection` action (see `DropCollection` struct in [dropcollection.hpp](src/api/options/dropcollection.hpp)):
* `writeConcern` - *document*.  The write concern for the operation.
  * `journal` - *boolean*.  If `true` confirms that the database has written the data to the on-disk journal before
    reporting a write operations was successful. This ensures that data is not lost if the database shuts down unexpectedly.
  * `nodes` - *integer*.  Sets the number of nodes that are required to acknowledge the write before the operation is
    considered successful. Write operations will block until they have been replicated to the specified number of
    servers in a replica set.
  * `acknowledgeLevel` - *integer*.  Sets the [acknowledgement](https://www.mongodb.com/docs/manual/reference/write-concern/#w-option) level for the write operation.
    * `0` - Represent the implicit default write concern.
    * `1` - Represent write concern with `w: "majority"`.
    * `2` - Represent write concern with `w: <custom write concern name>`.
    * `3` - Represent write concern for un-acknowledged writes.
    * `4` - Represent write concern for acknowledged writes.
  * `majority` - *integer*.  The amount of time (milliseconds) to wait before the write operation times out if it
    cannot reach the majority of nodes in the replica set. If the value is zero, then no timeout is set.
  * `tag` - *string*.  Sets the name representing the server-side `getLastErrorMode` entry containing the list of
    nodes that must acknowledge a write operation before it is considered a success.
  * `timeout` - *integer*.  Sets an upper bound on the time (milliseconds) a write concern can take to be satisfied.
    If the write concern cannot be satisfied within the timeout, the operation is considered a failure.

#### Index
The `document` represents the specification for the *index* to be created.
Additional options for the index (such as *unique*) can be specified via the
optional `options` sub-document.

See `Index` struct in [index.hpp](src/api/model/request/index.hpp)

Sample response (see `Index` struct in [index.hpp](src/api/model/response/index.hpp)):
```json
{"name": "unused_1"}
```

##### Options
The following options are supported for the `index` action (see `Index` struct in [index.hpp](src/api/options/index.hpp)):
* `collation` - *document*.  Sets the collation for this operation.
* `background` - *boolean*.  Whether or not to build the index in the *background* so that building the index does not
  block other database activities. The default is to build indexes in the *foreground*.
* `unique` - *boolean*.  Whether or not to create a *unique index* so that the collection will not accept insertion 
  of documents where the index key or keys match an existing value in the index.
* `hidden` - *boolean*.  Whether or not the index is hidden from the query planner. A hidden index is not evaluated
  as part of query plan selection.
* `name` - *string*.  The name of the index.
* `sparse` - *boolean*.  Whether or not to create a *sparse index*. Sparse indexes only reference documents with the indexed fields.
* `expireAfterSeconds` - *integer*.  Set a value, in seconds, as a TTL to control how long MongoDB retains documents in the collection.
* `version` - *integer*.  Sets the index version.
* `weights` - *document*.  For text indexes, sets the weight document. The weight document contains field and weight pairs.
* `defaultLanguage` - *string*.  For text indexes, the language that determines the list of stop words and the rules for the stemmer and tokenizer.
* `languageOverride` - *string*.  For text indexes, the name of the field, in the collection’s documents, that contains
  the override language for the document.
* `partialFilterExpression` - *document*.  Sets the document for the partial filter expression for partial indexes.
* `twodSphereVersion` - *integer*.  For 2dsphere indexes, the 2dsphere index version number. Version can be either 1 or 2.
* `twodBitsPrecision` - *integer* (0-255).  For 2d indexes, the precision of the stored geohash value of the location data.
* `twodLocationMin` - *double*.  For 2d indexes, the lower inclusive boundary for the longitude and latitude values.
* `twodLocationMax` - *double*.  For 2d indexes, the upper inclusive boundary for the longitude and latitude values.

#### Drop Index
The `document` represents the *index* specification for the 
[dropIndexes](https://docs.mongodb.com/master/reference/command/dropIndexes/#dbcmd.dropIndexes)
command. Additional options for the index (such as *write concern*) can be specified
via the optional `options` sub-document.

One of the following properties **must** be specified in the `document`:
* `name` - The `name` of the *index* to drop.  Should be a `string` value.
* `specification` - The full document specification of the index that was created.

See `DropIndex` struct in [dropindex.hpp](src/api/model/request/dropindex.hpp)

Sample response (see `DropIndex` struct in [index.hpp](src/api/model/response/index.hpp)):
```json
{"dropIndex": true}
```

##### Options
Same as [index](#index)

#### Bulk Write
Bulk insert/delete documents.  Corresponding version history documents for
inserted and/or deleted documents are created unless `skipVersion` is specified.

The *documents* to insert or delete in bulk must be specified as
*BSON array* properties in the `document` part of the payload.  Multiple arrays
may be specified as appropriate.

* `insert` - Array of documents which are to be inserted.  All documents **must**
  have a BSON ObjectId `_id` property.
* `remove` - Array of document specifications which represent the deletes.  Deletes
  are slow since the query specifications are used to retrieve the documents being
  deleted and create the corresponding version history documents.  Retrieving the
  documents in a loop adds significant processing time.  For example the bulk
  delete test (deleting 10000 documents) takes about 15 seconds to run.

Sample bulk create payload (see `Bulk` struct in [bulk.hpp](src/api/model/request/bulk.hpp)):
```json
{
  "action": "bulk",
  "database": "itest",
  "collection": "test",
  "document": {
    "insert": [{
      "_id": {
        "$oid": "5f6ba5f9de326c57bd64efb1"
      },
      "key": "value1"
    },
    {
      "_id": {
        "$oid": "5f6ba5f9de326c57bd64efb2"
      },
      "key": "value2"
    }],
    "remove": [{
      "_id": {
        "$oid": "5f6ba5f9de326c57bd64efb1"
      }
    }]
  }
}
```

Sample response for the above payload (see `Bulk` struct in [bulk.hpp](src/api/model/response/bulk.hpp)):
```json
{ "create" : 2, "history": 3, "remove" : 1 }
```

#### Aggregation Pipeline
Basic support for using *aggregation pipeline* features.  This feature will be expanded as use cases expand over a period of time.

The `document` in the payload **must** include a `specification` *array* of documents which correspond to the `match`,
`lookup` ... specifications for the aggregation pipeline operation (*stage*).  The matching documents 
will be returned in a `results` array in the response.

The following operators have been tested:
* `$match`
* `$lookup`
* `$unwind`
* `$group`
* `$sort`
* `$limit`
* `$project`
* `$addFields`
* `$facet`
* `$search` - Note requirement for `search` to be the first stage in a pipeline.
* `$unionWith`

Sample request payload (see `Pipeline` struct in [pipeline.hpp](src/api/model/request/pipeline.hpp)):
```json
{
  "action": "pipeline",
  "database": "itest",
  "collection": "test",
  "document": {
    "specification": [
      {"$match": {"_id": {"$oid": "5f861c8452c8ca000d60b783"}}},
      {"$sort": {"_id":  -1 }},
      {"$limit": 20},
      {"$lookup": {
        "localField": "user._id",
        "from": "user",
        "foreignField": "_id",
        "as": "users"
      }},
      {"$lookup": {
        "localField": "group._id",
        "from": "group",
        "foreignField": "_id",
        "as": "groups"
      }}
    ]
  }
}
```

Response structure is the same as for the `retrieve` [command](#retrieve).

#### Transaction
Execute a sequence of actions in a **transaction**.  Nest the individual actions
that are to be performed in the **transaction** within the `document` sub-document.

The `document` in the payload **must** include an `items` *array* of documents.
Each document in the array represents the full specification for the *action* in
the *transaction*.  The *document* specification is the same as the
*document* specification for using the service.

The specification for the *action* document in the `items` array is (see `TransactionBuilder` struct
in [transaction.hpp](src/api/model/request/transaction.hpp)):
* `action (string)` - The type of action to perform.  Should be one of `create|update|delete`.
* `database (string)` - The database in which the *step* is to be performed.
* `collection (string)` - The collection in which the *step* is to be performed.
* `document (document)` - The BSON specification for executing the `action`.
* `skipVersion (bool)` - Do not create version history document for this action.

The response to a **transaction** request has the following structure (see `Transaction` 
struct in [transaction.hpp](src/api/model/response/transaction.hpp)):
* `created (int)` - The number of documents that were *created* in this transaction.
* `updated (int)` - The number of documents that were *updated* in this transaction.
* `deleted (int)` - The number of documents that were *deleted* in this transaction.
* `history (document)` - Metadata about version history documents that were created.
    * `database (string)` - The database used to store version history data.
    * `collection (string)` - The collection used to store version history data.
    * `created (array<oid>)` - History document object ids for new documents created in the transaction.
    * `updated (array<oid>)` - History document object ids for documents updated in the transaction.
    * `deleted (array<oid>)` - History document object ids for documents deleted in the transaction.

See [samples](transaction.md) for sample request/response payloads from the
integration test suite.

##### Limitation
Update at present in only partially supported.  Strong assumption is made that
the document being updated is a full replacement.  In other words, there is a
strong assumption that the document includes the `_id` property, and that the
intention is to replace the existing document.

#### Create Timeseries
The `document` as specified will be inserted into the specified `database` and
timeseries `collection`.  The **BSON ObjectId** property/field (`_id`) may be omitted 
in the document.  The response will include the server generated `_id` for the inserted document
if using *acknowledged* writes.  No version history is created for timeseries data.

Sample request payload (see `CreateTimeseries` struct in [createtimeseries.hpp](src/api/model/request/createtimeseries.hpp)):
```json
{
  "action": "createTimeseries",
  "database": "<database name>",
  "collection": "<collection name>",
  "document": {
    "value": 123.456,
    "tags": {
      "property1": "string",
      "property2": false
    },
    "timestamp": "2024-11-21T17:36:28Z"
  }
}
```

Sample response payload when document is created (see `Create` struct in [create.hpp](src/api/model/request/create.hpp)):
```json
{
  "_id": {
    "$oid": "5f35e5e19e48c37186539141"
  },
  "database": "versionHistory",
  "collection": "entities"
}
```

##### Options
The following options are supported for the `createTimeseries` action (subset of `Insert` struct in [insert.hpp](src/api/options/insert.hpp)):
* `writeConcern` - *document*.  The write concern for the operation.
  * `journal` - *boolean*.  If `true` confirms that the database has written the data to the on-disk journal before
    reporting a write operations was successful. This ensures that data is not lost if the database shuts down unexpectedly.
  * `nodes` - *integer*.  Sets the number of nodes that are required to acknowledge the write before the operation is
    considered successful. Write operations will block until they have been replicated to the specified number of
    servers in a replica set.
  * `acknowledgeLevel` - *integer*.  Sets the [acknowledgement](https://www.mongodb.com/docs/manual/reference/write-concern/#w-option) level for the write operation.
    * `0` - Represent the implicit default write concern.
    * `1` - Represent write concern with `w: "majority"`.
    * `2` - Represent write concern with `w: <custom write concern name>`.
    * `3` - Represent write concern for un-acknowledged writes.
    * `4` - Represent write concern for acknowledged writes.
  * `majority` - *integer*.  The amount of time (milliseconds) to wait before the write operation times out if it
    cannot reach the majority of nodes in the replica set. If the value is zero, then no timeout is set.
  * `tag` - *string*.  Sets the name representing the server-side `getLastErrorMode` entry containing the list of
    nodes that must acknowledge a write operation before it is considered a success.
  * `timeout` - *integer*.  Sets an upper bound on the time (milliseconds) a write concern can take to be satisfied.
    If the write concern cannot be satisfied within the timeout, the operation is considered a failure.

#### Create Collection
Create a `collection` in the specified `database`.  If a `collection` already exists in the
`database` with the same *name*, an error is returned.  This is primarily useful when clients
wish to specify additional options when creating a collection (eg. create a timeseries collection).

Sample create collection payload (see `CreateCollection` struct in [createcollection.hpp](src/api/model/request/createcollection.hpp)):
```json
{
  "action": "createCollection",
  "database": "itest",
  "collection": "timeseries",
  "document": {
    "timeseries": {
      "timeField" : "date",
      "metaField": "tags",
      "granularity": "minutes"
    }
  }
}
```

Sample response payload (see `CreateCollection` struct in [collection.hpp](src/api/model/response/collection.hpp)):
```json
{
  "database": "itest",
  "collection": "timeseries"
}
```

##### Options
Options are not needed for the `createCollection` action.  Instead, the normal `document` payload 
is used as the options when creating the collection.  Refer the [mongodb](https://www.mongodb.com/docs/manual/reference/method/db.createCollection/)
documentation for the supported options.

#### Rename Collection
Rename a `collection` in the specified `database`.  If a `collection` already exists with the
`document.target` *name*, an error is returned.  The option to automatically drop a pre-existing
collection as supported by **MongoDB** is not supported.  For such cases, use the `dropCollection`
action prior to invoking this action.  Specify the *write concern* settings in the
optional `options` sub-document.

This is a potentially heavy-weight operation.  All *version history* documents for the specified
*database::collection* combination are also updated.  Version history document update is performed
*asynchronously*.  The operation enqueues an update operation to the version history documents, and
returns.  This can lead to queries against version history returning stale information for a short
period of time.

**Note**: Renaming the collection in all associated *version history* documents may be the *wrong* choice.
In chronological terms, those documents were associated with the previous `collection`.  Only future revisions
are associated with the renamed `target`.  However, this can create issues in terms of retrieval, or if
iterating over records for some other purpose, or if a new collection with the *previous* name is created
in future.

Sample rename payload (see `RenameCollection` struct in [renamecollection.hpp](src/api/model/request/renamecollection.hpp)):
```json
{
  "action": "renameCollection",
  "database": "itest",
  "collection": "test",
  "document": {"target": "test-renamed"}
}
```

Sample response payload (see `CreateCollection` struct in [collection.hpp](src/api/model/response/collection.hpp)):
```json
{
  "database": "itest",
  "collection": "test-renamed"
}
```

##### Options
Same write options as for the `dropCollection` [action](#drop-collection).

### Document Response
Create, update and delete actions only return some meta information about the
action that was performed.  The assumption is that caller already has all the
document information needed, and there is no need for the service to return
that information.

The `retrieve` action of course returns the results of executing the *database
query* encapsulated in the `document` property of the request payload.  The
following document model is returned as the response:
* `error` - A `string` value in case an error was encountered as a result of
  executing the *query*.  Caller should always check for existence of this property.
* `result` - A *BSON document* that is returned if the *query* included an
  `_id` property.  In such a case it is assumed that the query is a lookup for
  a single document.
* `results` - A *BSON array* with *document(s)* that were retrieved from the
  database for the *query*.
  
See [transaction](transaction.md) for sample request/response payloads for transaction requests.
  
### Limitation
At present only documents with **BSON ObjectId** `_id` is supported.  Streaming responses
(`cursor` interface) is not supported.

## Metrics
Metrics are collected for all requests to the service (unless client specifies `skipMetric`).  Metrics may be
stored in MongoDB itself, or a service that supports the [ILP](https://docs.influxdata.com/influxdb/v2.7/reference/syntax/line-protocol/).

### MongoDB
Metrics are collected in the specified `database` and `collection` combination
(or their defaults).  No TTL index is set on this (as it is left to the user's
requirements).  A `date` property is stored to create a TTL index as required.

The schema for a metric is as follows:
```json
{
  "_id": {"$oid": "5fd4b7e55f1ba96a695d1446"},
  "action": "retrieve",
  "database": "wpreading2",
  "collection": "databaseVersion",
  "size": 88,
  "time": 414306,
  "timestamp": 437909021088978,
  "date": {"$date": 437909021},
  "application": "bootstrap"
}
```

* **action** - The database action performed by the client.
* **database** - The database against which the action was performed.
* **collection** - The collection against which the action was performed.
* **size** - The total size of the response document.
* **time** - The time in `nanoseconds` for the action (includes any interaction with version history).
* **timestamp** - The time since UNIX epoch in `nanoseconds` for use when exporting to other timeseries databases.
* **date** - The BSON date at which the metric was created.  Use to define a TTL index as appropriate.
* **application** - The application that invoked the service if specified in the
  request payload.

### ILP
Metrics may be stored in a time series database of choice that supports the ILP.  We have only tested
storing metrics in [QuestDB](https://questdb.io/).  All the fields (except the `_id`) are stored in
the TSDB.  The `duration`, and `size` values are stored as *field sets* and the other values stored as *tag sets*.
The *name* for the series (*measurement*) can be specified via the command line argument, or will default to
the name of the `metrics` collection.

## Serialisation
A simple serialisation framework is also provided.  Uses the
[visit_struct](https://github.com/cbeck88/visit_struct) library to automatically serialise and deserialise
*visitable* classes/structs.

### BSON
A simple [serialisation](src/common/util/serialise.h) framework to serialise and deserialise
*visitable* classes/structs to and from BSON.  See the [test suite](test/unit/serialise.cpp) for sample use
of the framework.

The framework provides the following primary functions to handle (de)serialisation:
* `marshall<Model>( const Type& )` - to marshall the specified object to a BSON document.
* `unmarshall<Model>( bsoncxx::document::view view )` - unmarshall the BSON document into a default constructed object.
* `unmarshall<Model>( Model& m, bsoncxx::document::view view )` - unmarshall the BSON document into the specified model instance.

The framework handles non-visitable members within a visitable root object.  Custom implementations can be implemented.
* For non-visitable classes/structs, implement the following functions as appropriate:
  * `bsoncxx::types::bson_value::value bson( const <Class/Struct Type>& model )` that will produce a BSON document as a value variant for the data encapsulated in the object.
  * `void set( <Class/Struct Type>& field, bsoncxx::types::bson_value::view value )` that will populate the model instance from the BSON value variant.
* For partially visitable classes/structs, implement the following `populate` callback functions as appropriate:
  * `void populate( const <Class/Struct Type>& model, bsoncxx::builder::stream::document& doc )` to add the non-visitable fields to the BSON stream builder.
  * `void populate( <Class/Struct Type>& model, bsoncxx::document::view view )` to populate the non-visitable fields in the object from the BSON document.

### JSON
A simple [serialisation](src/common/util/json.h) framework to serialise and deserialise *visitable* classes/structs
to and from JSON.  See the [test suite](test/unit/json.cpp) for sample use of the framework.

Similar to the BSON framework, the JSON framework also handles non-visitable members within a visitable root object.
Custom implementations can be implemented.
* For non-visitable classes/structs, implement the following functions as appropriate:
  * `boost::json::value json( const <Class/Struct Type>& model )` that will produce a JSON value for the data encapsulated in the object.
  * `void set( const char* name, <Class/Struct Type>& field, simdjson::ondemand::value& value )` that will populate the model instance from the JSON value.
* For partially visitable classes/structs, implement the following `populate` callback functions as appropriate:
  * `void populate( const <Class/Struct Type>& model, boost::json::object& object )` to add non-visitable fields to the JSON object.
  * `void populate( const <Class/Struct Type>& model, simdjson::ondemand::object& object )` to populate non-visitable fields from the JSON object.

#### Validation
JSON input mostly comes via HTTP from untrusted sources.  Consequently, there is a need for validating the JSON
input.  Basic support for input validation is provided via a `validate` function.

A `validate( const char*, M& )` function is defined.  This is to for validating the JSON input being parsed. A
default specialisation is provided for `std::string` fields.  This rejects strings with more than `40%` (configurable)
special characters.  Users are advised to implement specific implementations specific to their domain requirements.
Users may also use environment variables to influence the default implementation.
* `JSON_PARSE_VALIDATION_IGNORE` - Environment variable that expects a comma or space separated list of
  field names that should be ignored by the validator.  Default values are `password, version`.  Example:
  `export SPT_JSON_PARSE_VALIDATION_IGNORE='password, file, version, firmware, identifier'`
* `SPT_JSON_PARSE_VALIDATION_RATIO` - Environment variable (`double`) that sets the maximum allowed ratio of
  special characters in the input string.  Default is `0.4`.  Example: `export SPT_JSON_PARSE_VALIDATION_RATIO='0.35'`

## Testing
Integration tests for the service will be developed in a few different languages
to ensure full interoperability.  The test suites will be available under the
`test` directory or under the `client` directory. The following suites are present at present:
* `C++`
    * **Integration** - Integration test suite under the `test/integration` directory.
    * **Performance** - Performance test suite under the `test/performance` directory.
* `Python` - See [features](client/python/features) for the test suite.
* `Julia` - See [test](client/julia/MongoService/test/runtests.jl) for the test suite.
* `go` - Simple test program under the `test/go` directory.
    ```shell script
    (cd mongo-service/test/go; go build -o /tmp/gomongo; /tmp/gomongo)
    ```

### Connection Pool
A simple connection pool [implementation](src/api/pool/pool.h) is provided
in the api, along with its associated [test suite](test/integration/pool.cpp).
The implementation is based on a *factory* function that can create valid connections
as needed.

The pool is managed using a `std::deque`.  Connections returned to the pool are
added to the *back* (least idle), while acquiring a connection pops it from the
*front* (most idle) of the `deque`.
  
#### Configuration
Configuration is via a simple structure for common options such as initial size,
max pool size, max connection size, and maximum idle time for a connection.  It
supports a simple validity check (via a mandatory `bool valid()` function)
of the connection before adding it back to the pool.

A `maxIdleTime` property is used to close connections that have been idling more
than the specified time.  The `initialSize` property is also used as a *minimum*
pool size configuration.  The *minimum* size is always maintained regardless of
*idle* time.

#### Proxy
Acquiring a connection from the pool returns a `std::optional<Proxy>` instance.
If the maximum number of connections has been reached, `std::nullopt` is returned.
The *Proxy* implements RAII by returning the connection to the pool when the
instance is destroyed.

#### ConnectionWrapper
A wrapper is used to associate a last used timestamp to the *connection*.  This
is used to enforce maximum idle time policy on the underlying *connection*.

### Sample Client
A sample async client using coroutines with connection pool implementation is
available under the [client](test/client) directory.

### Performance Test
The performance test suite performs a simple *CRUD* operation using the service.
Each test creates a document, retrieves the document, updates the document and
finally deletes the document.  All operations other than *retrieve* will create
an associated version history document in the database.  All operations also
create a corresponding metric document in the database.  Thus a *CRUD* operation
involves approximately 12 database operations internally.

The tests are set up to *run* each set of *CRUD* operations `10` times (*iterations*),
and a *run* is repeated a second time to get better average and variability numbers.
Separate runs are set up with `10, 50, 100, 500` and `1000` concurrent threads.
All tests are against the simple [docker stack](docker/stack.yml)
running on the same machine.  A key goal of the test is to ensure that there are
no errors while running the test.

The following numbers as recorded on my laptop during normal use (plenty of
other applications and processes running) and with the Docker daemon restricted
to using half the available CPU cores.

```shell script
[==========] Running 5 benchmarks.
[ RUN      ] SocketClientFixture.crud(int concurrency = 10) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 10) (1356.777305 ms)
[   RUNS   ]        Average time: 678388.652 us (~303172.494 us)
                    Fastest time: 464013.326 us (-214375.326 us / -31.601 %)
                    Slowest time: 892763.979 us (+214375.327 us / +31.601 %)
                     Median time: 678388.652 us (1st quartile: 464013.326 us | 3rd quartile: 892763.979 us)
                                  
             Average performance: 1.47408 runs/s
                Best performance: 2.15511 runs/s (+0.68103 runs/s / +46.20025 %)
               Worst performance: 1.12012 runs/s (-0.35396 runs/s / -24.01254 %)
              Median performance: 1.47408 runs/s (1st quartile: 2.15511 | 3rd quartile: 1.12012)
                                  
[ITERATIONS]        Average time: 67838.865 us (~30317.249 us)
                    Fastest time: 46401.333 us (-21437.533 us / -31.601 %)
                    Slowest time: 89276.398 us (+21437.533 us / +31.601 %)
                     Median time: 67838.865 us (1st quartile: 46401.333 us | 3rd quartile: 89276.398 us)
                                  
             Average performance: 14.74081 iterations/s
                Best performance: 21.55111 iterations/s (+6.81029 iterations/s / +46.20025 %)
               Worst performance: 11.20117 iterations/s (-3.53964 iterations/s / -24.01254 %)
              Median performance: 14.74081 iterations/s (1st quartile: 21.55111 | 3rd quartile: 11.20117)
[ RUN      ] SocketClientFixture.crud(int concurrency = 50) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 50) (3786.378171 ms)
[   RUNS   ]        Average time: 1893189.086 us (~131220.814 us)
                    Fastest time: 1800401.958 us (-92787.127 us / -4.901 %)
                    Slowest time: 1985976.213 us (+92787.127 us / +4.901 %)
                     Median time: 1893189.086 us (1st quartile: 1800401.958 us | 3rd quartile: 1985976.213 us)
                                  
             Average performance: 0.52821 runs/s
                Best performance: 0.55543 runs/s (+0.02722 runs/s / +5.15369 %)
               Worst performance: 0.50353 runs/s (-0.02468 runs/s / -4.67212 %)
              Median performance: 0.52821 runs/s (1st quartile: 0.55543 | 3rd quartile: 0.50353)
                                  
[ITERATIONS]        Average time: 189318.909 us (~13122.081 us)
                    Fastest time: 180040.196 us (-9278.713 us / -4.901 %)
                    Slowest time: 198597.621 us (+9278.713 us / +4.901 %)
                     Median time: 189318.909 us (1st quartile: 180040.196 us | 3rd quartile: 198597.621 us)
                                  
             Average performance: 5.28209 iterations/s
                Best performance: 5.55432 iterations/s (+0.27222 iterations/s / +5.15369 %)
               Worst performance: 5.03531 iterations/s (-0.24679 iterations/s / -4.67212 %)
              Median performance: 5.28209 iterations/s (1st quartile: 5.55432 | 3rd quartile: 5.03531)
[ RUN      ] SocketClientFixture.crud(int concurrency = 100) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 100) (6902.353423 ms)
[   RUNS   ]        Average time: 3451176.712 us (~85190.281 us)
                    Fastest time: 3390938.086 us (-60238.626 us / -1.745 %)
                    Slowest time: 3511415.337 us (+60238.625 us / +1.745 %)
                     Median time: 3451176.712 us (1st quartile: 3390938.086 us | 3rd quartile: 3511415.337 us)
                                  
             Average performance: 0.28976 runs/s
                Best performance: 0.29490 runs/s (+0.00515 runs/s / +1.77646 %)
               Worst performance: 0.28479 runs/s (-0.00497 runs/s / -1.71551 %)
              Median performance: 0.28976 runs/s (1st quartile: 0.29490 | 3rd quartile: 0.28479)
                                  
[ITERATIONS]        Average time: 345117.671 us (~8519.028 us)
                    Fastest time: 339093.809 us (-6023.863 us / -1.745 %)
                    Slowest time: 351141.534 us (+6023.863 us / +1.745 %)
                     Median time: 345117.671 us (1st quartile: 339093.809 us | 3rd quartile: 351141.534 us)
                                  
             Average performance: 2.89756 iterations/s
                Best performance: 2.94904 iterations/s (+0.05147 iterations/s / +1.77646 %)
               Worst performance: 2.84785 iterations/s (-0.04971 iterations/s / -1.71551 %)
              Median performance: 2.89756 iterations/s (1st quartile: 2.94904 | 3rd quartile: 2.84785)
[ RUN      ] SocketClientFixture.crud(int concurrency = 500) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 500) (37825.690178 ms)
[   RUNS   ]        Average time: 18912845.089 us (~77556.025 us)
                    Fastest time: 18858004.698 us (-54840.391 us / -0.290 %)
                    Slowest time: 18967685.480 us (+54840.391 us / +0.290 %)
                     Median time: 18912845.089 us (1st quartile: 18858004.698 us | 3rd quartile: 18967685.480 us)
                                  
             Average performance: 0.05287 runs/s
                Best performance: 0.05303 runs/s (+0.00015 runs/s / +0.29081 %)
               Worst performance: 0.05272 runs/s (-0.00015 runs/s / -0.28913 %)
              Median performance: 0.05287 runs/s (1st quartile: 0.05303 | 3rd quartile: 0.05272)
                                  
[ITERATIONS]        Average time: 1891284.509 us (~7755.602 us)
                    Fastest time: 1885800.470 us (-5484.039 us / -0.290 %)
                    Slowest time: 1896768.548 us (+5484.039 us / +0.290 %)
                     Median time: 1891284.509 us (1st quartile: 1885800.470 us | 3rd quartile: 1896768.548 us)
                                  
             Average performance: 0.52874 iterations/s
                Best performance: 0.53028 iterations/s (+0.00154 iterations/s / +0.29081 %)
               Worst performance: 0.52721 iterations/s (-0.00153 iterations/s / -0.28913 %)
              Median performance: 0.52874 iterations/s (1st quartile: 0.53028 | 3rd quartile: 0.52721)
[ RUN      ] SocketClientFixture.crud(int concurrency = 1000) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 1000) (70436.871301 ms)
[   RUNS   ]        Average time: 35218435.650 us (~1036096.530 us)
                    Fastest time: 34485804.768 us (-732630.883 us / -2.080 %)
                    Slowest time: 35951066.533 us (+732630.883 us / +2.080 %)
                     Median time: 35218435.650 us (1st quartile: 34485804.768 us | 3rd quartile: 35951066.533 us)
                                  
             Average performance: 0.02839 runs/s
                Best performance: 0.02900 runs/s (+0.00060 runs/s / +2.12444 %)
               Worst performance: 0.02782 runs/s (-0.00058 runs/s / -2.03786 %)
              Median performance: 0.02839 runs/s (1st quartile: 0.02900 | 3rd quartile: 0.02782)
                                  
[ITERATIONS]        Average time: 3521843.565 us (~103609.653 us)
                    Fastest time: 3448580.477 us (-73263.088 us / -2.080 %)
                    Slowest time: 3595106.653 us (+73263.088 us / +2.080 %)
                     Median time: 3521843.565 us (1st quartile: 3448580.477 us | 3rd quartile: 3595106.653 us)
                                  
             Average performance: 0.28394 iterations/s
                Best performance: 0.28997 iterations/s (+0.00603 iterations/s / +2.12444 %)
               Worst performance: 0.27816 iterations/s (-0.00579 iterations/s / -2.03786 %)
              Median performance: 0.28394 iterations/s (1st quartile: 0.28997 | 3rd quartile: 0.27816)
[==========] Ran 5 benchmarks.
```

## Build
Check out the sources and use `cmake` to build and install the project locally.

### UNIX

<details>
<summary>Install <a href="https://boost.org/">Boost</a></summary>

```shell
BOOST_VERSION=1.83.0
INSTALL_DIR=/usr/local/boost

cd /tmp
ver=`echo "${BOOST_VERSION}" | awk -F'.' '{printf("%d_%d_%d",$1,$2,$3)}'`
curl -OL https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${ver}.tar.bz2
tar xfj boost_${ver}.tar.bz2
sudo rm -rf $INSTALL_DIR
cd boost_${ver} \
  && ./bootstrap.sh \
  && sudo ./b2 -j8 cxxflags=-std=c++20 install link=static threading=multi runtime-link=static --prefix=$INSTALL_DIR --without-python --without-mpi
```
</details>

<details>
<summary>Install <a href="https://mongocxx.org/">mongocxx</a> driver</summary>

```shell
PREFIX=/usr/local/mongo
MONGOC_VERSION=1.24.2
MONGOCXX_VERSION=3.8.0

sudo rm -rf $PREFIX
cd /tmp
sudo rm -rf mongo-c-driver*
curl -L -O https://github.com/mongodb/mongo-c-driver/releases/download/${MONGOC_VERSION}/mongo-c-driver-${MONGOC_VERSION}.tar.gz
tar xzf mongo-c-driver-${MONGOC_VERSION}.tar.gz
cd /tmp/mongo-c-driver-${MONGOC_VERSION}
mkdir cmake-build && cd cmake-build
cmake \
  -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PREFIX \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_SHARED_LIBS=OFF \
  -DENABLE_SASL=OFF \
  -DENABLE_TESTS=OFF \
  -DENABLE_EXAMPLES=OFF \
  ..
make -j8
sudo make install

cd /tmp
sudo rm -rf mongo-cxx-driver
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r${MONGOCXX_VERSION}/mongo-cxx-driver-r${MONGOCXX_VERSION}.tar.gz
tar -xzf mongo-cxx-driver-r${MONGOCXX_VERSION}.tar.gz
cd mongo-cxx-driver-r${MONGOCXX_VERSION}/build
cmake \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_INSTALL_PREFIX=$PREFIX \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_TESTING_ENABLED=OFF \
  -DENABLE_TESTS=OFF \
  -DBSONCXX_POLY_USE_STD=ON \
  ..
make -j8
sudo make install
```
</details>

<details>
<summary>Check out, build and install the project.</summary>

```shell
cd /var/tmp
git clone https://github.com/sptrakesh/mongo-service.git
cd mongo-service
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/usr/local/boost \
  -DCMAKE_PREFIX_PATH=/usr/local/mongo \
  -DCMAKE_INSTALL_PREFIX=/usr/local/spt \
  -DBUILD_TESTING=OFF -S . -B build
cmake --build build -j12
sudo cmake --install build
```
</details>

### Windows

Install dependencies to build the project.  The following instructions at times reference `arm` or `arm64` architecture.  Modify
those values as appropriate for your hardware.  These instructions are based on steps I followed to set up the project on a
Windows 11 virtual machine running via Parallels Desktop on a M2 Mac.

<details>
<summary>Install <a href="https://boost.org/">Boost</a></summary>

* Download and extract Boost 1.82 (or above) to a temporary location (eg. `\opt\src`).
* Launch the Visual Studio Command utility and cd to the temporary location.
```shell
cd \opt\src
curl -OL https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz
tar -xfz boost_1_82_0.tar.gz
cd boost_1_82_0
.\bootstrap.bat
.\b2 -j8 install threading=multi address-model=64 architecture=arm asynch-exceptions=on --prefix=\opt\local --without-python --without-mpi

cd ..
del /s /q boost_1_82_0
rmdir /s /q boost_1_82_0
```
</details>

<details>
<summary>Install <a href="https://mongocxx.org/">mongocxx</a> driver</summary>

* Download Mongo C Driver (1.24.2 or above) and extract sources to a temporary location.
* Launch the Visual Studio Command utility and cd to the temporary location.
```shell
cd \opt\src
curl -OL https://github.com/mongodb/mongo-c-driver/releases/download/1.24.2/mongo-c-driver-1.24.2.tar.gz
tar -xfz mongo-c-driver-1.24.2.tar.gz
cd mongo-c-driver-1.24.2
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=c:\opt\local -DBUILD_SHARED_LIBS=OFF -DENABLE_SASL=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -S . -B cmake-build
cmake --build cmake-build --target install --parallel 8
cd ..
del /s /q mongo-c-driver-1.24.2
rmdir /s /q mongo-c-driver-1.24.2
```

* Downlaod Mongo CXX Driver (3.8 or above) and extract sources to a temporary location.
* Launch the Visual Studio Command utility and cd to the temporary location.
```shell
cd \opt\src
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.8.0/mongo-cxx-driver-r3.8.0.tar.gz
tar -xvf mongo-cxx-driver-r3.8.0.tar.gz
cd mongo-cxx-driver-r3.8.0
cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=c:\opt\local -DCMAKE_INSTALL_PREFIX=c:\opt\local -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF -DCMAKE_TESTING_ENABLED=OFF -DENABLE_TESTS=OFF -DBSONCXX_POLY_USE_STD=ON -S . -B build
cmake --build build --target install --parallel 8
cd ..
del /s /q mongo-cxx-driver-r3.8.0
rmdir /s /q mongo-cxx-driver-r3.8.0
```
</details>

<details>
<summary>Install <a href="https://github.com/fmtlib/fmt">fmt</a> library</summary>

Launch the Visual Studio Command utility and cd to a temporary location.
```shell
cd \opt\src
git clone https://github.com/fmtlib/fmt.git --branch 9.1.0
cd fmt
cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\opt\local -DCMAKE_INSTALL_LIBDIR=lib -DFMT_TEST=OFF -DFMT_MODULE=ON -S . -B build
cmake --build build --target install -j8
```
</details>

<details>
<summary>Install <a href="https://github.com/ericniebler/range-v3">range-v3</a> library</summary>

Launch the Visual Studio Command utility and cd to a temporary location.
```shell
git clone https://github.com/ericniebler/range-v3.git --branch 0.12.0
cd range-v3
cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\opt\local -DCMAKE_INSTALL_LIBDIR=lib -DRANGE_V3_DOCS=OFF -DRANGE_V3_EXAMPLES=OFF -DRANGE_V3_PERF=OFF -DRANGE_V3_TESTS=OFF -DRANGE_V3_INSTALL=ON -B build -S .
cmake --build build --target install -j8
```
</details>

<details>
<summary>Install <a href="https://github.com/Microsoft/vcpkg">vcpkg</a> manager</summary>

Launch the Visual Studio Command utility.
```shell
cd \opt\src
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat -disableMetrics
.\vcpkg integrate install --vcpkg-root \opt\src\vcpkg
.\vcpkg install curl:arm64-windows
.\vcpkg install cpr:arm64-windows
```
</details>

<details>
<summary>Check out, build and install the project.</summary>

Launch the Visual Studio Command utility.
```shell
cd %homepath%\source\repos
git clone https://github.com/sptrakesh/mongo-service.git
cd mongo-service
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=\opt\local -DCMAKE_INSTALL_PREFIX=\opt\spt -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE="C:/opt/src/vcpkg/scripts/buildsystems/vcpkg.cmake" -S . -B build
cmake --build build -j8
cmake --build build --target install
```
</details>

#### Running
Run the service by specifying options similar to the following:
```shell
cd %homepath%\source\repos\mongo-service
build\src\service\Debug\mongo-service.exe -e true -o %temp%\ -p 2020 -m "mongodb://test:test@192.168.0.39/admin?authSource=admin&compressors=snappy&w=1" -l debug --metric-batch-size 2
```

Run other targets such as `unitTest`, `integration`, `client` as appropriate directly from the IDE.

#### Limitations
The following limitations have been encountered when running the test suites.  A few tests
have been disabled when running on Windows to avoid running into these issues.

* Cannot create an index with a specific name.  For some reason, this leads to the index name
  being stored with non-UTF-8 characters, which then causes further issues down the road.

## Clients
Sample clients in other languages that use the service.
* **Julia** - Sample client package for [Julia](https://julialang.org) is available under the [julia](client/julia) directory.
* **Python** - Sample client package for [Python](https://python.org) is available under the [python](client/python) directory.

### API Usage
The [API](src/api/api.hpp) can be used to communicate with the TCP service.  Initialise the library
(`init` function) before using the other api functions.  A higher level abstraction is also provided
via the [repository.hpp](src/api/repository/repository.hpp) interface.

Client code bases can use [cmake](https://cmake.org/) to link against the library.

```shell
# In your CMakeLists.txt file
find_package(MongoService REQUIRED COMPONENTS api)
if (APPLE)
  include_directories(/usr/local/spt/include)
else()
  include_directories(/opt/spt/include)
endif (APPLE)
target_link_libraries(${Target_Name} PRIVATE mongo-service::api ...)

# Run cmake
if [ `uname` = "Darwin" ]
then
  cmake -DCMAKE_PREFIX_PATH=/usr/local/boost;/usr/local/mongo;/usr/local/spt -S . -B build
else
  cmake -DCMAKE_PREFIX_PATH=/opt/local;/opt/spt -S . -B build
fi
cmake --build build -j12
```

### Command Line Utility
A simple command line utility is available for generating BSON ObjectId values.  This utility is installed as
`bin/genoid` under the destination `bin` directory.
* Run without any arguments to generate a ObjectId at current time.
* Run with `--at <ISO Format date-time>`.  Example: `/usr/local/spt/bin/genoid --at 2024-10-26T07:28:57Z`

## Acknowledgements
The following components are used to build this project:
* **[Boost:Asio](https://github.com/boostorg/asio)** - We use *Asio* for the `TCP socket` server implementation.
* **[MongoCXX](https://mongocxx.org/)** - MongoDB C++ driver.
* **[magic_enum](https://github.com/Neargye/magic_enum) - Static reflection for enums.
* **[visit_struct](https://github.com/cbeck88/visit_struct)** - Struct visitor library used for the [serialisation](src/common/util/serialise.h) utility functions.
* **[concurrentqueue](https://github.com/cameron314/concurrentqueue)** - Lock free concurrent queue implementation for metrics.
* **[NanoLog](https://github.com/Iyengar111/NanoLog)** - Logging framework used for the server. I modified the implementation for daily rolling log files.
* **[Catch2](https://github.com/catchorg/Catch2)** - Unit testing framework.
* **[Clara](https://github.com/catchorg/Clara)** - Command line options parser.
* **[hayai](https://github.com/nickbruun/hayai)** - Performance testing framework.
