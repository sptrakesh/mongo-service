import json

from addict import Dict
from behave import given, then, step
from behave.api.async_step import async_run_until_complete
from behave.runner import Context
from bson import ObjectId
from bson.json_util import dumps
from hamcrest import assert_that, equal_to, not_, greater_than

from client import has_key as _has_key
from logger import log as _log
from request import bulk_request, count_request

_database = "itest"
_collection = "test"


@given("Connection for bulk operations")
def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    context.oid1 = ObjectId()
    context.oid2 = ObjectId()


@then("Create documents in bulk")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid1"), "Document id1 not set")
    assert_that(hasattr(context, "oid2"), "Document id2 not set")
    doc = Dict()
    doc.insert = []
    doc.insert.append(Dict({"_id": context.oid1, "key": "value1"}))
    doc.insert.append(Dict({"_id": context.oid2, "key": "value2"}))
    req = bulk_request(database=_database, collection=_collection, document=doc)
    resp = await context.client.execute(req)
    _log.info(f"Bulk create response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("create", resp), "Response did not contain create")
    assert_that(_has_key("history", resp), "Response did not contain history")
    assert_that(_has_key("delete", resp), "Response did not contain delete")


@step("Retrieve count of documents after bulk insert")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = count_request(database=_database, collection=_collection)
    resp = await context.client.execute(req)
    _log.info(f"Count response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("count", resp), "Response did not contain count")

    context.count = resp.count


@step("Delete documents in bulk")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oid1"), "Document id1 not set")
    assert_that(hasattr(context, "oid2"), "Document id2 not set")
    doc = Dict()
    doc.delete = [Dict({"_id": context.oid1}), Dict({"_id": context.oid2})]
    req = bulk_request(database=_database, collection=_collection, document=doc)
    resp = await context.client.execute(req)
    _log.info(f"Bulk delete response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("create", resp), "Response did not contain create")
    assert_that(_has_key("history", resp), "Response did not contain history")
    assert_that(_has_key("delete", resp), "Response did not contain delete")


@step("Retrieve count of documents after bulk delete")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    req = count_request(database=_database, collection=_collection)
    resp = await context.client.execute(req)
    _log.info(f"Count response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("count", resp), "Response did not contain count")
    assert_that(context.count, greater_than(resp.count), "Bulk delete did not reduce document count")


@step("Create a large batch of documents")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    context.oids = []
    doc = Dict()
    doc.insert = []
    for i in range(10000):
        context.oids.append(ObjectId())
        doc.insert.append(Dict({"_id": context.oids[i], "iter": i}))
        doc.insert[i].key1 = "value1"
        doc.insert[i].key2 = "value2"
        doc.insert[i].key3 = "value3"
        doc.insert[i].key4 = "value3"
        doc.insert[i].key5 = "value3"

    req = bulk_request(database=_database, collection=_collection, document=doc)
    resp = await context.client.execute(req)
    _log.info(f"Bulk create with large batch response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("create", resp), "Response did not contain create")
    assert_that(resp.create, equal_to(len(doc.insert)), "Created count not equal to number of documents")
    assert_that(_has_key("history", resp), "Response did not contain history")
    assert_that(resp.history, equal_to(len(doc.insert)), "Created history count not equal to number of documents")
    assert_that(_has_key("delete", resp), "Response did not contain delete")
    assert_that(resp.delete, equal_to(0), "Deleted count not equal to 0")


@step("Delete a large batch of documents")
@async_run_until_complete
async def step_impl(context: Context):
    assert_that(hasattr(context, "client"), "Client not set")
    assert_that(hasattr(context, "oids"), "Batch ids not set")
    doc = Dict()
    doc.delete = []
    for oid in context.oids:
        doc.delete.append(Dict({"_id": oid}))
    req = bulk_request(database=_database, collection=_collection, document=doc)
    resp = await context.client.execute(req)
    _log.info(f"Bulk delete with large batch response: {dumps(resp)}")

    assert_that(_has_key("error", resp), not_(True), "Response contained error")
    assert_that(_has_key("create", resp), "Response did not contain create")
    assert_that(resp.create, equal_to(0), "Created count not equal to 0")
    assert_that(_has_key("history", resp), "Response did not contain history")
    assert_that(resp.history, equal_to(len(doc.delete)), "Created history count not equal to number of documents")
    assert_that(_has_key("delete", resp), "Response did not contain delete")
    assert_that(resp.delete, equal_to(len(doc.delete)), "Deleted count not equal to number of documents")
