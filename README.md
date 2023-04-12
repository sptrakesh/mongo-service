# Mongo Service

* [Command Line Options](#command-line-options)
* [Version History](#version-history)
* [Protocol](#protocol)
    * [Document Payload](#document-payload)
        * [Create](#create)
        * [Retrieve](#retrieve)
        * [Count](#count)
        * [Update](#update)
        * [Delete](#delete)
        * [Index](#index)
        * [Drop Index](#drop-index)
        * [Bulk Write](#bulk-write)
        * [Aggregation Pipeline](#aggregation-pipeline)
        * [Transaction](#transaction)
    * [Document Response](#document-response)
    * [Options](#options)
    * [Limitation](#limitation)
* [Metrics](#metrics)
* [Testing](#testing)
    * [Connection Pool](#connection-pool)
    * [Performance Test](#performance-test)
* [Build](#build)
* [Clients](#clients)
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
to.  Specify via the `-s` or `--metrics-database` option.  Default `versionHistory`.
* `metricsCollection` - The collection in which to store the metric documents.
Specify via the `-t` or `--metrics-collection` option.  Default `metrics`.
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
  `create|retrieve|update|delete|count|index|dropCollection|dropIndex|bulk|pipeline|transaction`.
* `database (string)` - The Mongo database the action is to be performed against.
    - Not needed for `transaction` action.
* `collection (string)` - The Mongo collection the action is to be performed against.
    - Not needed for `transaction` action.
* `document (document)` - The document payload to associate with the database operation.
  For `create` and `update` this is assumed to be the documet that is being saved.
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

Sample request payload:
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

Sample response payload when version history document is created (default option):
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

**Note:** The `_id` in the response is the object id of the version history
document that was created.

Sample response payload when version history document is not created:
```json
{
  "_id": {
    "$oid": "5f35e5e19e48c37186539141"
  },
  "skipVersion": true
}
```

**Note:** The `_id` in the response is the object id for the document as specified
in the input payload.

#### Retrieve
Retrieve obviously does not have any interaction with the version history system
(unless you are retrieving versions).  We provide this since one of the other
purposes behind this service is to route/proxy all datastore interactions via
this service.

Sample retrieve payload:
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

Sample response:
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

#### Count
Count the number of documents matching the specified query document.
The query document can be empty to get the count of all documents in the specified
collection.

Sample count payload:
```json
{
  "action": "count",
  "database": "itest",
  "collection": "test",
  "document": {}
}
```

Sample response:
```json
{ "count" : 11350 }
```

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

Sample update request by `_id`:
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

Sample response:
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
if an attempt is made to replace multiple documents (the query filter must return
a single document).  A version history document is created with the replaced
document.

The following sample shows an example of performing a `replace` action.
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

##### Update Document
If the `document` has a `update` sub-document, then existing document(s) are
updated with the information contained in it.  This is a *merge* operation where
only the fields specified in the `update` are set on the candidate document(s).
A version history document is created for each updated document.

If the input `filter` sub-document has an `_id` property, and is of type BSON
Object Id, then a single document update is made.

###### Update with unset
As a simplification, it is possible to omit the `$set` operator, if used in conjunction with a `$unset` operator.  All
top level properties other than `_id` and `$unset` are implicitly added to a `$set` document in the actual update document
sent to MongoDB.

Sample request payload with explicit `$set`:
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
 
#### Delete
The `document` represents the *query* to execute to find the candidate documents
to delete from the `database`:`collection`.  The query is executed to retrieve
the candidate documents, and the documents removed from the specified
`database:collection`.  The retrieved documents are then written to the version
history database.

Sample delete request:
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

Sample delete response:
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

#### Drop Collection
Drop the specified collection and all its containing documents.  Specify an
empty `document` in the payload to satisfy payload requirements.  Specify the
*write concern* settings in the optional `options` sub-document.

Sample count payload:
```json
{
  "action": "dropCollection",
  "database": "itest",
  "collection": "test",
  "document": {}
}
```

Sample response:
```json
{ "dropCollection" : true }
```

#### Index
The `document` represents the specification for the *index* to be created.
Additional options for the index (such as *unique*) can be specified via the
optional `options` sub-document.

#### Drop Index
The `document` represents the *index* specification for the 
[dropIndexes](https://docs.mongodb.com/master/reference/command/dropIndexes/#dbcmd.dropIndexes)
command. Additional options for the index (such as *write concern*) can be specified
via the optional `options` sub-document.

One of the following properties **must** be specified in the `document`:
* `name` - The `name` of the *index* to drop.  Should be a `string` value.
* `specification` - The full document specification of the index that was created.

#### Bulk Write
Bulk insert/delete documents.  Corresponding version history documents for
inserted and/or deleted documents are created unless `skipVersion` is specified.

The *documents* to insert or delete in bulk must be specified as
*BSON array* properties in the `document` part of the payload.  Multiple arrays
may be specified as appropriate.

* `insert` - Array of documents which are to be inserted.  All documents **must**
  have a BSON ObjectId `_id` property.
* `delete` - Array of document specifications which represent the deletes.  Deletes
  are slow since the query specifications are used to retrieve the documents being
  deleted and create the corresponding version history documents.  Retrieving the
  documents in a loop adds significant processing time.  For example the bulk
  delete test (deleting 10000 documents) takes about 15 seconds to run.

Sample bulk create payload:
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
    "delete": [{
      "_id": {
        "$oid": "5f6ba5f9de326c57bd64efb1"
      }
    }]
  }
}
```

Sample response for the above payload:
```json
{ "create" : 2, "history": 3, "delete" : 1 }
```

#### Aggregation Pipeline
Basic support for using *aggregation pipeline* features.  This feature will be
expanded as use cases expand over a period of time.

The `document` in the payload **must** include a `specification` *array* of
documents which correspond to the `match`, `lookup` ...
specifications for the aggregation pipeline operation.  The matching documents 
will be returned in a `results` array in the response.

The following operators are supported:
* `$match`
* `$lookup`
* `$unwind`
* `$group`
* `$sort`
* `$limit`
* `$project`
* `$addFields`
* `$facet`

Sample request payload:
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

#### Transaction
Execute a sequence of actions in a **transaction**.  Nest the individual actions
that are to be performed in the **transaction** within the `document` sub-document.

The `document` in the payload **must** include an `items` *array* of documents.
Each document in the array represents the full specification for the *action* in
the *transaction*.  The *document* specification is the same as the
*document* specification for using the service.

The specification for the *action* document in the `items` array is:
* `action (string)` - The type of action to perform.  Should be one of `create|update|delete`.
* `database (string)` - The database in which the *step* is to be performed.
* `collection (string)` - The collection in which the *step* is to be performed.
* `document (document)` - The BSON specification for executing the `action`.
* `skipVersion (bool)` - Do not create version history document for this action.

The response to a **transaction** request has the following structure:
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
  
See [transaction](transaction.md) for sample request/response payloads for
transaction requests.
  
### Options
Options specified in the request payload are parsed into the appropriate
`option` document for the specified `action`.

### Limitation
At present only documents with **BSON ObjectId** `_id` is supported.

## Metrics
Metrics are collected in the specified `database` and `collection` combination
(or their defaults).  No TTL index is set on this (as it is left to the user's
requirements).  A `date` property is stored to create a TTL index as required.

The schema for a metric is as follows:
```json
{
  "_id": ObjectId("5fd4b7e55f1ba96a695d1446"),
  "action": "retrieve",
  "database": "wpreading2",
  "collection": "databaseVersion",
  "size": 88,
  "time": 414306,
  "timestamp": 437909021088978,
  "date": Date(437909021),
  "application": "bootstrap"
}
```

* **action** - The database action performed by the client.
* **database** - The database against which the action was performed.
* **collection** - The collection against which the action was performed.
* **size** - The total size of the response document.
* **time** - The time in `nanoseconds` for the action (includes any interaction
  with version history).
* **timestamp** - The time since UNIX epoch in `nanoseconds` for use when exporting
  to other timeseries databases.
* **date** - The BSON date at which the metric was created.  Use to define a TTL
  index as appropriate.
* **application** - The application that invoked the service if specified in the
  request payload.

## Testing
Integration tests for the service will be developed in a few different languages
to ensure full interoperability.  The test suites will be available under the
`test` directory. The following suites are present at present:
* `C++`
    * **Integration** - Integration test suite under the `test/integration` directory.
    * **Performance** - Performance test suite under the `test/performance` directory.
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

```shell
git clone https://github.com/sptrakesh/mongo-service.git
cd mongo-service
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/usr/local/boost \
  -DCMAKE_PREFIX_PATH=/usr/local/mongo \
  -DCMAKE_INSTALL_PREFIX=/usr/local/spt \
  -DBUILD_TESTING=OFF -S . -B build
cmake --build build -j12
(cd build; sudo make install)
```

## Clients
Sample clients in other languages that use the service.
* **Julia** - Sample client package for [Julia](https://julialang.org) is available
  under the [julia](client/julia) directory/

## Acknowledgements
This software has been developed mainly using work other people/projects have contributed.
The following are the components used to build this software:
* **[Boost:Asio](https://github.com/boostorg/asio)** - We use *Asio* for the
`TCP socket` server implementation.
* **[MongoCXX](http://mongocxx.org/)** - MongoDB C++ driver.
* **[concurrentqueue](https://github.com/cameron314/concurrentqueue)** - Lock
  free concurrent queue implementation for metrics.
* **[NanoLog](https://github.com/Iyengar111/NanoLog)** - Logging framework used
for the server.  I modified the implementation for daily rolling log files.
* **[Clara](https://github.com/catchorg/Clara)** - Command line options parser.
* **[Catch2](https://github.com/catchorg/Catch2)** - Unit testing framework.
* **[hayai](https://github.com/nickbruun/hayai)** - Performance testing framework.
