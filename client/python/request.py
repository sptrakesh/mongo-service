from enum import Enum, auto
from typing import Optional

from addict import Dict
from bson import encode


class Action(Enum):
    create = auto()
    retrieve = auto()
    update = auto()
    delete = auto()
    count = auto()
    index = auto()
    dropCollection = auto()
    dropIndex = auto()
    bulk = auto()
    pipeline = auto()
    transaction = auto()


class Request:
    def __init__(self, database: str, collection: str, document: Dict, action: Action,
                 options: Optional[Dict] = None, metadata: Optional[Dict] = None,
                 correlation_id: str = "", skip_version: bool = False):
        self._database = database
        self._collection = collection
        self._document = document
        self._action = action
        self._options = options
        self._metadata = metadata
        self._correlation_id = correlation_id
        self._skip_version = skip_version

    def bson(self) -> bytes:
        d = Dict()
        d.database = self._database
        d.collection = self._collection
        d.document = self._document
        d.action = self._action.name

        if self._options:
            d.options = self._options
        if self._metadata:
            d.metadata = self._metadata
        if self._correlation_id:
            d.correlationId = self._correlation_id
        if self._skip_version:
            d.skipVersion = self._skip_version

        return encode(d)


def create_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                   metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.create,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def retrieve_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                     metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.retrieve,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def update_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                   metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.update,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def delete_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                   metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.delete,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def count_request(database: str, collection: str, options: Optional[Dict] = None,
                  metadata: Optional[Dict] = None, correlation_id: str = "") -> Request:
    return Request(database=database, collection=collection, document=Dict(), action=Action.count,
                   options=options, metadata=metadata, correlation_id=correlation_id)


def index_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                  metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.index,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def drop_index_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                       metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.dropIndex,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def drop_collection_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                            metadata: Optional[Dict] = None, correlation_id: str = "",
                            skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.dropCollection,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def bulk_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                 metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.bulk,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def pipeline_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                     metadata: Optional[Dict] = None, correlation_id: str = "", skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.pipeline,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)


def transaction_request(database: str, collection: str, document: Dict, options: Optional[Dict] = None,
                        metadata: Optional[Dict] = None, correlation_id: str = "",
                        skip_version: bool = False) -> Request:
    return Request(database=database, collection=collection, document=document, action=Action.transaction,
                   options=options, metadata=metadata, correlation_id=correlation_id, skip_version=skip_version)
