# Mongo Service

* [Version History](#version-history)
* [Protocol](#protocol)
    * [Document Payload](#document-payload)
        * [Create](#create)
        * [Retrieve](#retrieve)
        * [Count](#count)
        * [Update](#update)
        * [Delete](#delete)
        * [Index](#index)
    * [Document Response](#document-response)
    * [Options](#options)
    * [Limitation](#limitation)
* [Acknowledgements](#acknowledgements)
    
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
* `action` - The type of database action being performed.  One of 
  `create|retrieve|update|delete|count|index`.
* `database` - The Mongo database the action is to be performed against.
* `collection` - The Mongo collection the action is to be performed against.
* `document` - The document payload to associate with the database operation.
  For `create` and `update` this is assumed to be the documet that is being saved.
  For `retrieve` or `count` this is the *query* to execute.  For `delete` this
  is a simple `document` with an `_id` field.
* `options` - The options to associate with the Mongo request.  These correspond
  to the appropriate options as used by the Mongo driver.
* `metadata` - Optional *metadata* to attach to the version history document that
  is created (not relevant for `retrieve` obviously).  This typically will include
  information about the user performing the action, any other information as
  relevant in the system that uses this service.
* `application` - Optional name of the *application* accessing the service.
  Helps to retrieve database metrics for a specific *application*.
* `correlationId` - Optional *correlation id* to associate with the metric record
  created by this action.  This is useful for tracing logs originating from a single
  operation/request within the overall system.  This value is stored as a *string*
  value in the *metric* document to allow for sensible data types to used as the
  *correlation id*.
* `skipVersion` - Optional `bool` value to indicate not to create a *version history*
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

#### Retrieve
Retrieve obviously does not have any interaction with the version history system
(unless you are retrieving versions).  We provide this since one of the other
purposes behind this service is to route/proxy all datastore interactions via
this service.

#### Count
Count the number of documents matching the specified query document.
The query document can be empty to get the count of all documents in the specified
collection.

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
* *Multi-Document* - For multi-document updates, a list BSON object ids for
successful updates (`success`), failed updates (`failure`), and the basic
information about the version history documents (`history`).
 
##### Update By Id
The simple and direct update use case.  If the `document` has an `_id` property,
the remaining properties are merged into the stored document.  A version history
document with the resulting stored document is also created.

##### Replace Document
If the `document` has a `replace` sub-document, then the existing document as
specified by the `filter` query will be replaced.  MongoDB will return as error
if an attempt is made to replace multiple documents (the query filter must return
a single document).  A version history document is created with the replaced
document.

##### Update Document
If the `document` has a `update` sub-document, then existing document(s) are
updated with the information contained in it.  This is a *merge* operation where
only the fields specified in the `update` are set on the candidate document(s).
A version history document is created for each updated document.
 
#### Delete
The `document` represents the *query* to execute to find the candidate documents
to delete from the `database`:`collection`.  The query is executed to retrieve
the candidate documents, and the documents removed from the specified
`database:collection`.  The retrieved documents are then written to the version
history database.

#### Index
The `document` represents the specification for the *index* to be created.
Additional options for the index (such as *unique*) can be specified via the
optional `options` sub-document.

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
  
### Options
Options specified in the request payload are parsed into the appropriate
`option` document for the specified `actCMakeLists.txtion`.

### Limitation
At present only documents with **BSON ObjectId** `_id` is supported.

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

## Acknowledgements
This software has been developed mainly using work other people have contributed.
The following are the components used to build this software:
* **[Boost:Asio](https://github.com/boostorg/asio)** - We use *Asio* for the
`TCP socket` server implementation.
* **[MongoCXX](http://mongocxx.org/)** - MongoDB C++ driver.
* **[NanoLog](https://github.com/Iyengar111/NanoLog)** - Logging framework used
for the server.  I modified the implementation for daily rolling log files.
* **[Clara](https://github.com/catchorg/Clara)** - Command line options parser.
* **[Catch2](https://github.com/catchorg/Catch2)** - Unit testing framework.
* **[hayai](https://github.com/nickbruun/hayai)** - Performance testing framework.
