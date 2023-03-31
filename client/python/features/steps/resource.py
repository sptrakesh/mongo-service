from behave import given, then
from behave.api.async_step import async_run_until_complete
from behave.runner import Context
from bson.json_util import dumps
from hamcrest import assert_that, equal_to, not_

from client import Client as _Client, has_key as _has_key
from logger import log as _log
from request import count_request

_database = "itest"
_collection = "test"


@given("Mongo service running on {host} on port {port:d}")
def step_impl(context: Context, host: str, port: int):
    context.host = host
    context.port = port


@then("Client can be used as an auto-closing resource")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "host"), equal_to(True), "Mongo service host not set")
    assert_that(hasattr(context, "port"), equal_to(True), "Mongo service port not set")

    async with _Client(host=context.host, port=context.port, application="python-test") as client:
        req = count_request(database=_database, collection=_collection)
        resp = await client.execute(req)
        _log.info(f"Count response: {dumps(resp)}")

        assert_that(_has_key("error", resp), not_(True), "Response contained error")
        assert_that(_has_key("count", resp), "Response did not contain count")
