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
* [Testing](#testing)
    * [Performance Test](#performance-test)
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
`option` document for the specified `action`.

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

### Performance Test
The performance test suite performs a simple *CRUD* operation using the service.
Each test creates a document, retrieves the document, updates the document and
finally deletes the document.  All operations other than *retrieve* will create
an associated version history document in the database.  All operations also
create a corresponding metric document in the database.  Thus a *CRUD* operation
involves approximately 12 database operations internally.

The tests are set up to *run* each *CRUD* operations `10` times, and a *run*
is repeated a second time to get better average and variability numbers.  Seperate
runs are set up with `10, 50, 100, 500` and `1000` concurrent threads.  All
testing is conducted against the simple [docker stack](docker/stack.yml) running
on the same machine.  A key goal of the test is to ensure that no errors are
encountered while running the test.

The following numbers were achieved on my laptop during normal use (plenty of
other applications and processes running).

```shell script
[==========] Running 5 benchmarks.
[ RUN      ] SocketClientFixture.crud(int concurrency = 10) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 10) (1244.758409 ms)
[   RUNS   ]        Average time: 622379.204 us (~161627.488 us)
                    Fastest time: 508091.312 us (-114287.893 us / -18.363 %)
                    Slowest time: 736667.097 us (+114287.892 us / +18.363 %)
                     Median time: 622379.204 us (1st quartile: 508091.312 us | 3rd quartile: 736667.097 us)
                                  
             Average performance: 1.60674 runs/s
                Best performance: 1.96815 runs/s (+0.36141 runs/s / +22.49357 %)
               Worst performance: 1.35747 runs/s (-0.24927 runs/s / -15.51418 %)
              Median performance: 1.60674 runs/s (1st quartile: 1.96815 | 3rd quartile: 1.35747)
                                  
[ITERATIONS]        Average time: 62237.920 us (~16162.749 us)
                    Fastest time: 50809.131 us (-11428.789 us / -18.363 %)
                    Slowest time: 73666.710 us (+11428.789 us / +18.363 %)
                     Median time: 62237.920 us (1st quartile: 50809.131 us | 3rd quartile: 73666.710 us)
                                  
             Average performance: 16.06737 iterations/s
                Best performance: 19.68150 iterations/s (+3.61413 iterations/s / +22.49357 %)
               Worst performance: 13.57465 iterations/s (-2.49272 iterations/s / -15.51418 %)
              Median performance: 16.06737 iterations/s (1st quartile: 19.68150 | 3rd quartile: 13.57465)
[ RUN      ] SocketClientFixture.crud(int concurrency = 50) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 50) (5826.898797 ms)
[   RUNS   ]        Average time: 2913449.399 us (~366662.599 us)
                    Fastest time: 2654179.788 us (-259269.610 us / -8.899 %)
                    Slowest time: 3172719.009 us (+259269.610 us / +8.899 %)
                     Median time: 2913449.399 us (1st quartile: 2654179.788 us | 3rd quartile: 3172719.009 us)
                                  
             Average performance: 0.34324 runs/s
                Best performance: 0.37676 runs/s (+0.03353 runs/s / +9.76835 %)
               Worst performance: 0.31519 runs/s (-0.02805 runs/s / -8.17184 %)
              Median performance: 0.34324 runs/s (1st quartile: 0.37676 | 3rd quartile: 0.31519)
                                  
[ITERATIONS]        Average time: 291344.940 us (~36666.260 us)
                    Fastest time: 265417.979 us (-25926.961 us / -8.899 %)
                    Slowest time: 317271.901 us (+25926.961 us / +8.899 %)
                     Median time: 291344.940 us (1st quartile: 265417.979 us | 3rd quartile: 317271.901 us)
                                  
             Average performance: 3.43236 iterations/s
                Best performance: 3.76764 iterations/s (+0.33528 iterations/s / +9.76835 %)
               Worst performance: 3.15187 iterations/s (-0.28049 iterations/s / -8.17184 %)
              Median performance: 3.43236 iterations/s (1st quartile: 3.76764 | 3rd quartile: 3.15187)
[ RUN      ] SocketClientFixture.crud(int concurrency = 100) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 100) (14476.523693 ms)
[   RUNS   ]        Average time: 7238261.846 us (~268039.996 us)
                    Fastest time: 7048728.948 us (-189532.899 us / -2.618 %)
                    Slowest time: 7427794.745 us (+189532.899 us / +2.618 %)
                     Median time: 7238261.846 us (1st quartile: 7048728.948 us | 3rd quartile: 7427794.745 us)
                                  
             Average performance: 0.13815 runs/s
                Best performance: 0.14187 runs/s (+0.00371 runs/s / +2.68889 %)
               Worst performance: 0.13463 runs/s (-0.00353 runs/s / -2.55167 %)
              Median performance: 0.13815 runs/s (1st quartile: 0.14187 | 3rd quartile: 0.13463)
                                  
[ITERATIONS]        Average time: 723826.185 us (~26804.000 us)
                    Fastest time: 704872.895 us (-18953.290 us / -2.618 %)
                    Slowest time: 742779.475 us (+18953.290 us / +2.618 %)
                     Median time: 723826.185 us (1st quartile: 704872.895 us | 3rd quartile: 742779.475 us)
                                  
             Average performance: 1.38155 iterations/s
                Best performance: 1.41870 iterations/s (+0.03715 iterations/s / +2.68889 %)
               Worst performance: 1.34629 iterations/s (-0.03525 iterations/s / -2.55167 %)
              Median performance: 1.38155 iterations/s (1st quartile: 1.41870 | 3rd quartile: 1.34629)
[ RUN      ] SocketClientFixture.crud(int concurrency = 500) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 500) (94160.116860 ms)
[   RUNS   ]        Average time: 47080058.430 us (~15328630.910 us)
                    Fastest time: 36241079.567 us (-10838978.863 us / -23.022 %)
                    Slowest time: 57919037.293 us (+10838978.863 us / +23.022 %)
                     Median time: 47080058.430 us (1st quartile: 36241079.567 us | 3rd quartile: 57919037.293 us)
                                  
             Average performance: 0.02124 runs/s
                Best performance: 0.02759 runs/s (+0.00635 runs/s / +29.90799 %)
               Worst performance: 0.01727 runs/s (-0.00397 runs/s / -18.71402 %)
              Median performance: 0.02124 runs/s (1st quartile: 0.02759 | 3rd quartile: 0.01727)
                                  
[ITERATIONS]        Average time: 4708005.843 us (~1532863.091 us)
                    Fastest time: 3624107.957 us (-1083897.886 us / -23.022 %)
                    Slowest time: 5791903.729 us (+1083897.886 us / +23.022 %)
                     Median time: 4708005.843 us (1st quartile: 3624107.957 us | 3rd quartile: 5791903.729 us)
                                  
             Average performance: 0.21240 iterations/s
                Best performance: 0.27593 iterations/s (+0.06353 iterations/s / +29.90799 %)
               Worst performance: 0.17265 iterations/s (-0.03975 iterations/s / -18.71402 %)
              Median performance: 0.21240 iterations/s (1st quartile: 0.27593 | 3rd quartile: 0.17265)
[ RUN      ] SocketClientFixture.crud(int concurrency = 1000) (2 runs, 10 iterations per run)
[     DONE ] SocketClientFixture.crud(int concurrency = 1000) (214198.956681 ms)
[   RUNS   ]        Average time: 107099478.340 us (~2445070.413 us)
                    Fastest time: 105370552.471 us (-1728925.869 us / -1.614 %)
                    Slowest time: 108828404.210 us (+1728925.869 us / +1.614 %)
                     Median time: 107099478.340 us (1st quartile: 105370552.471 us | 3rd quartile: 108828404.210 us)
                                  
             Average performance: 0.00934 runs/s
                Best performance: 0.00949 runs/s (+0.00015 runs/s / +1.64081 %)
               Worst performance: 0.00919 runs/s (-0.00015 runs/s / -1.58867 %)
              Median performance: 0.00934 runs/s (1st quartile: 0.00949 | 3rd quartile: 0.00919)
                                  
[ITERATIONS]        Average time: 10709947.834 us (~244507.041 us)
                    Fastest time: 10537055.247 us (-172892.587 us / -1.614 %)
                    Slowest time: 10882840.421 us (+172892.587 us / +1.614 %)
                     Median time: 10709947.834 us (1st quartile: 10537055.247 us | 3rd quartile: 10882840.421 us)
                                  
             Average performance: 0.09337 iterations/s
                Best performance: 0.09490 iterations/s (+0.00153 iterations/s / +1.64081 %)
               Worst performance: 0.09189 iterations/s (-0.00148 iterations/s / -1.58867 %)
              Median performance: 0.09337 iterations/s (1st quartile: 0.09490 | 3rd quartile: 0.09189)
[==========] Ran 5 benchmarks.
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
