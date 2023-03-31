from behave.runner import Context
from behave.model import Feature
from behave.api.async_step import async_run_until_complete

from client import Client as _Client
from logger import log


@async_run_until_complete
async def before_feature(context: Context, feature: Feature):
    log.info(f"Setting up feature {feature.name}")

    if feature.name == "Resource":
        return

    context.client = await _Client(host="localhost", port=2000, application="python-test")


@async_run_until_complete
async def after_feature(context: Context, feature: Feature):
    log.info(f"Cleaning up feature {feature.name}")

    if feature.name == "Resource":
        return

    if hasattr(context, "client"):
        await context.client.close()
