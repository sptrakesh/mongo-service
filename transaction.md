# Transaction
Sample payloads and responses for transactions against the service.  These are
all taken from the integration test suite.

**Note:** Transactions require acknowledged writes and MongoDB configured with
replica sets.  For testing a single node configured to be a replicat set is
sufficient (as illustrated in the [docker stack](docker/stack.yml) used for testing).

## Create and delete multiple documents.
Sample request and response payloads.

### Request
A request that creates and immediately deletes 2 documents.  This will result
in two version history documents created for each document.
```json
{
  "action": "transaction",
  "database": "itest",
  "collection": "test",
  "document": {
    "items": [{
      "action": "create",
      "database": "itest",
      "collection": "test",
      "document": {
        "key": "value1",
        "_id": {
          "$oid": "60f4537f35f77004094e73d5"
        }
      }
    }, {
      "action": "create",
      "database": "itest",
      "collection": "test",
      "document": {
        "key": "value2",
        "_id": {
          "$oid": "60f4537f35f77004094e73d6"
        }
      }
    }, {
      "action": "delete",
      "database": "itest",
      "collection": "test",
      "document": {
        "_id": {
          "$oid": "60f4537f35f77004094e73d5"
        }
      }
    }, {
      "action": "delete",
      "database": "itest",
      "collection": "test",
      "document": {
        "_id": {
          "$oid": "60f4537f35f77004094e73d6"
        }
      }
    }]
  }
}
```

### Response
The standard response structure with information about the number of documents
that were created, updated or deleted, and the version history document metadata
for each of those (if history not disabled).
```json
{
  "created": 2,
  "updated": 0,
  "deleted": 2,
  "history": {
    "database": "versionHistory",
    "collection": "entities",
    "created": [{
      "$oid": "60f4537fee94a4075a4c9518"
    }, {
      "$oid": "60f4537fee94a4075a4c9519"
    }],
    "deleted": [{
      "$oid": "60f4537fee94a4075a4c951a"
    }, {
      "$oid": "60f4537fee94a4075a4c951b"
    }]
  }
}
```