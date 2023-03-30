from addict import Dict
from behave import given, then, step
from behave.api.async_step import async_run_until_complete
from behave.runner import Context
from bson.json_util import dumps
from hamcrest import assert_that, equal_to, not_, less_than

from client import has_key as _has_key
from logger import log as _log
from request import index_request, drop_index_request

_database = "itest"
_collection = "test"


@given("A mongo service client for indexing")
def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")


@then("Create an index")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = index_request(database=_database, collection=_collection, document=Dict({"unused": 1}))
    resp = await context.client.execute(req)
    _log.info(f"Create index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("name", resp), "Response did not contain index name")


@step("Create the index again")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = index_request(database=_database, collection=_collection, document=Dict({"unused": 1}))
    resp = await context.client.execute(req)
    _log.info(f"Create index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("name", resp), not_(True), "Response contained index name")


@step("Drop the index")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = drop_index_request(database=_database, collection=_collection,
                             document=Dict({"specification": {"unused": 1}}))
    resp = await context.client.execute(req)
    _log.info(f"Drop index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("dropIndex", resp))
    assert_that(resp.dropIndex, equal_to(True))


@step("Create a unique index")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = index_request(database=_database, collection=_collection, document=Dict({"unused": 1}),
                        options=Dict({"name": "uniqueIndex", "unique": True, "expireAfterSeconds": 5}))
    resp = await context.client.execute(req)
    _log.info(f"Create unique index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("name", resp), "Response did not contain index name")


@step("Create a unique index again")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = index_request(database=_database, collection=_collection, document=Dict({"unused": 1}),
                        options=Dict({"name": "uniqueIndex", "unique": True, "expireAfterSeconds": 5}))
    resp = await context.client.execute(req)
    _log.info(f"Create unique index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("name", resp), not_(True), "Response contained index name")


@step("Drop the unique index")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = drop_index_request(database=_database, collection=_collection, document=Dict({"name": "uniqueIndex"}))
    resp = await context.client.execute(req)
    _log.info(f"Drop unique index response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("dropIndex", resp))
    assert_that(resp.dropIndex, equal_to(True))
