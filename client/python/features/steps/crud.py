from addict import Dict
from behave import given, then, step
from behave.api.async_step import async_run_until_complete
from behave.runner import Context
from bson import ObjectId
from bson.json_util import dumps
from hamcrest import assert_that, equal_to, not_, less_than

from client import has_key as _has_key
from logger import log as _log
from request import create_request, count_request, retrieve_request, update_request, delete_request

_database = "itest"
_collection = "test"


@given("A mongo service client")
def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")


@then("Create a document")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    context.oid = ObjectId()
    req = create_request(database=_database, collection=_collection, document=Dict({"_id": context.oid, "key": "value"}))
    resp = await context.client.execute(req)
    _log.info(f"Create response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("database", resp), "Response did not contain version history database")
    assert_that(_has_key("collection", resp), "Response did not contain version history collection")
    assert_that(_has_key("_id", resp), "Response did not contain version history id")
    assert_that(_has_key("entity", resp), "Response did not include created entity information")
    assert_that(resp.entity, equal_to(context.oid), "Response entity id not same as specified")

    context.vhdb = resp.database
    context.vhc = resp.collection
    context.vhid = resp["_id"]


@step("Retrieve count of documents")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "vhid"), "Version history id not set")
    req = count_request(database=_database, collection=_collection)
    resp = await context.client.execute(req)
    _log.info(f"Count response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("count", resp), "Response did not contain count")

    context.count = resp.count


@step("Retrieve the document by id")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = retrieve_request(database=_database, collection=_collection, document=Dict({"_id": context.oid}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("results", resp), not_(True), "Response contained results array")
    assert_that(_has_key("result", resp), "Response did not include result document")
    assert_that(_has_key("_id", resp.result), "Response result does not have id")
    assert_that(resp.result["_id"], equal_to(context.oid), "Response result id not same")
    assert_that(_has_key("key", resp.result), "Response result did not contain key")
    assert_that(resp.result.key, equal_to("value"), "Response result key value not same")


@step("Retrieve the document by property")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = retrieve_request(database=_database, collection=_collection, document=Dict({"key": "value"}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("result", resp), not_(True), "Response contained result document")
    assert_that(_has_key("results", resp), "Response did not contain results array")

    found = False
    for d in resp.results:
        assert_that(_has_key("_id", d), "Document does not have id")
        if d["_id"] == context.oid:
            found = True
    assert_that(found, "Created document not returned")


@step("Retrieve the version history document by id")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "vhdb"), "Version history database not set")
    assert_that(hasattr(context, "vhc"), "Version history collection not set")
    assert_that(hasattr(context, "vhid"), "Version history id not set")
    req = retrieve_request(database=context.vhdb, collection=context.vhc, document=Dict({"_id": context.vhid}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve version history response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("results", resp), not_(True), "Response contained results array")
    assert_that(_has_key("result", resp), "Response did not include result document")
    assert_that(_has_key("_id", resp.result), "Response result did not include id")
    assert_that(resp.result["_id"], equal_to(context.vhid), "Response result id not same")
    assert_that(_has_key("entity", resp.result), "Response result did not include entity")
    assert_that(_has_key("_id", resp.result.entity), "Response result entity did not include id")
    assert_that(resp.result.entity["_id"], equal_to(context.oid), "Response result entity id not same")


@step("Retrieve the version history document by entity id")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    assert_that(hasattr(context, "vhdb"), "Version history database not set")
    assert_that(hasattr(context, "vhc"), "Version history collection not set")
    assert_that(hasattr(context, "vhid"), "Version history document id not set")
    req = retrieve_request(database=context.vhdb, collection=context.vhc, document=Dict({"entity._id": context.oid}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve version history by entity id response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("result", resp), not_(True), "Response contained result document")
    assert_that(_has_key("results", resp), "Response did not contain results array")
    assert_that(len(resp.results), equal_to(1), "Response results array size wrong")
    assert_that(_has_key("_id", resp.results[0]), "Response results item did not contain id")
    assert_that(resp.results[0]["_id"], equal_to(context.vhid), "Response results item id not same as version history id")
    assert_that(_has_key("entity", resp.results[0]), "Response results item did not contain entity")
    assert_that(_has_key("_id", resp.results[0].entity), "Response results item entity did not contain id")
    assert_that(resp.results[0].entity["_id"], equal_to(context.oid), "Response results entity id not same")


@step("Update the document by id")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = update_request(database=_database, collection=_collection, document=Dict({"_id": context.oid, "key1": "value1"}))
    resp = await context.client.execute(req)
    _log.info(f"Update response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("skipVersion", resp), not_(True), "Response included skipVersion")
    assert_that(_has_key("document", resp), "Response did not include document")
    assert_that(_has_key("_id", resp.document), "Response document did not contain id")
    assert_that(resp.document["_id"], equal_to(context.oid), "Response document id not same")
    assert_that(_has_key("key", resp.document), "Response document does not have key")
    assert_that(resp.document.key, equal_to("value"), "Response document key value not same")
    assert_that(_has_key("key1", resp.document), "Response document did not contain key1")
    assert_that(resp.document.key1, equal_to("value1"), "Response document key1 value not same")


@step("Update the document without version history")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = update_request(database=_database, collection=_collection,
                         document=Dict({"_id": context.oid, "key1": "value1"}), skip_version=True)
    resp = await context.client.execute(req)
    _log.info(f"Update without history response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("skipVersion", resp), "Response did not contain skipVersion")
    assert_that(resp.skipVersion, "Response skipVersion not true")


@step("Delete the document")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = delete_request(database=_database, collection=_collection, document=Dict({"_id": context.oid}))
    resp = await context.client.execute(req)
    _log.info(f"Delete response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("success", resp), "Response did not include success")
    assert_that(_has_key("history", resp), "Response did not include history")


@step("Retrieve count of documents after delete")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "count"), "Original count not set")
    req = count_request(database=_database, collection=_collection)
    resp = await context.client.execute(req)
    _log.info(f"Count after response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("count", resp), "Response did not include count")
    assert_that(resp.count, less_than(context.count), "Response count not less than saved")
