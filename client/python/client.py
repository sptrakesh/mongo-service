from asyncio import open_connection
from addict import Dict
from bson import decode

from logger import log as _log
from request import Request


class Client:
    def __init__(self, host: str, port: int, application: str):
        assert host
        self._host = host
        assert port > 0
        self._port = port
        assert application
        self._application = application

    async def _async_init(self):
        self._reader, self._writer = await open_connection(host=self._host, port=self._port)
        _log.info(f"Connected to {self._host}:{self._port}")
        return self

    def __await__(self):
        return self._async_init().__await__()

    async def __aenter__(self):
        await self._async_init()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        await self.close()

    async def execute(self, request: Request) -> Dict:
        b = request.bson()
        _log.info(f"Writing {len(b)} bytes to server.")

        self._writer.write(b)
        await self._writer.drain()

        _log.info("Reading response size to 4 byte array")
        lv = await self._reader.read(4)
        l = int.from_bytes(lv, "little")
        _log.info(f"Response size: {l}")

        b = await self._reader.read(l - 4)

        ba = b''.join([lv, b])
        return Dict(decode(ba))

    async def close(self):
        self._writer.close()
        await self._writer.wait_closed()
        _log.info(f"Disconnected from {self._host}:{self._port}")


def has_key(key: str, dictionary: Dict):
    if dictionary is None:
        return False
    return key in dictionary and dictionary[key] is not None