# Python Client
A simple client in [Python](https://python.org) for the service.  The [Client](client.py) handles the
connection to the service.  A [Request](request.py) structure similar to the C++ API is to construct
requests to the made to the service.  A single `execute` method is implemented in the *Client* to send
requests to the service and gather the response.

See the integration test suite (e.g. [crud](features/steps/crud.py)) for sample use of the client.

## Resource
It is also possible to use the client as an auto-closing resource.

```python
async with Client(host="localhost", port=2000, application="python-test") as client:
    req = count_request(database="test", collection="test")
    resp = await client.execute(req)
    log.info(f"Count response: {bson.json_util.dumps(resp)}")
```