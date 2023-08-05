from asyncio import sleep
from addict import Dict
from behave import then, step
from behave.api.async_step import async_run_until_complete
from behave.runner import Context
from bson import ObjectId
from bson.json_util import dumps
from hamcrest import assert_that, equal_to, not_

from client import has_key as _has_key
from logger import log as _log
from request import create_request, drop_collection_request, rename_collection_request, retrieve_request

_database = "itest"
_collection = "oldname"
_target = "newname"


@then("Document created in a new collection")
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


@step("Cannot rename to existing collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    context.oid = ObjectId()
    req = rename_collection_request(database=_database, collection=_collection, document=Dict({"target": "test"}))
    resp = await context.client.execute(req)
    _log.info(f"Rename collection response: {dumps(resp)}")

    assert_that(_has_key("error", resp), "Response did not return error")


@step("Rename collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    context.oid = ObjectId()
    req = rename_collection_request(database=_database, collection=_collection, document=Dict({"target": _target}))
    resp = await context.client.execute(req)
    _log.info(f"Rename collection response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("database", resp), "Response did not contain input database")
    assert_that(_has_key("collection", resp), "Response did not contain target collection")


@step("Retrieve document from renamed collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    _log.info(f"Retrieve document by id: {context.oid}")
    await sleep(0.5)
    req = retrieve_request(database=_database, collection=_target, document=Dict({"_id": context.oid}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve response: {dumps(resp)}")

    """
    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("results", resp), not_(True), "Response contained results array")
    assert_that(_has_key("result", resp), "Response did not include result document")
    assert_that(_has_key("_id", resp.result), "Response result does not have id")
    assert_that(resp.result["_id"], equal_to(context.oid), "Response result id not same")
    assert_that(_has_key("key", resp.result), "Response result did not contain key")
    assert_that(resp.result.key, equal_to("value"), "Response result key value not same")
    """


@step("Retrieve version history for renamed collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "vhdb"), "Version history database not set")
    assert_that(hasattr(context, "vhc"), "Version history collection not set")
    assert_that(hasattr(context, "vhid"), "Version history id not set")
    _log.info(f"Retrieve version history by id: {context.vhid}")
    await sleep(0.5)
    req = retrieve_request(database=context.vhdb, collection=context.vhc,
                           document=Dict({"_id": context.vhid, "database": _database, "collection": _target}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve version history response: {dumps(resp)}")

    """
    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("results", resp), not_(True), "Response contained results array")
    assert_that(_has_key("result", resp), "Response did not include result document")
    assert_that(_has_key("_id", resp.result), "Response result did not include id")
    assert_that(resp.result["_id"], equal_to(context.vhid), "Response result id not same")
    assert_that(_has_key("entity", resp.result), "Response result did not include entity")
    assert_that(_has_key("_id", resp.result.entity), "Response result entity did not include id")
    assert_that(resp.result.entity["_id"], equal_to(context.oid), "Response result entity id not same")
    """


@step("Drop renamed collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid"), "Document id not set")
    req = drop_collection_request(database=_database, collection=_target, document=Dict({"clearVersionHistory": True}))
    resp = await context.client.execute(req)
    _log.info(f"Drop collection response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("results", resp), not_(True), "Response contained results array")
    assert_that(_has_key("result", resp), not_(True), "Response contained result document")
    assert_that(_has_key("dropCollection", resp), "Response did not include dropCollection")


@step("Revision history cleared for dropped collection")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "vhdb"), "Version history database not set")
    assert_that(hasattr(context, "vhc"), "Version history collection not set")
    assert_that(hasattr(context, "vhid"), "Version history id not set")
    req = retrieve_request(database=context.vhdb, collection=context.vhc,
                           document=Dict({"_id": context.vhid, "database": _database, "collection": _target}))
    resp = await context.client.execute(req)
    _log.info(f"Retrieve version history response: {dumps(resp)}")

    assert_that(_has_key("error", resp), "Response not error")