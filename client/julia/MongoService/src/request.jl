import LightBSON: bson_write

module Actions
@enum(Action::UInt8,
    create, retrieve, update, delete, count,
    index, dropCollection, dropIndex,
    bulk, pipeline, transaction
)
end

const Document = Dict{String,Any}

struct Request
    database::String
    collection::String
    document::Document
    action::Actions.Action
    options::Document
    metadata::Document
    correlationId::String
    skipVersion::Bool
    skipMetric::Bool
end

Request(;database::String, collection::String, document::Document, action::Actions.Action,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, action, options, metadata, correlationId, skipVersion, skipMetric)

CreateRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.create, options, metadata, correlationId, skipVersion, skipMetric)

RetrieveRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.retrieve, options, metadata, correlationId, skipVersion, skipMetric)

UpdateRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.update, options, metadata, correlationId, skipVersion, skipMetric)

DeleteRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.delete, options, metadata, correlationId, skipVersion, skipMetric)

CountRequest(;database::String, collection::String,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, Document(), Actions.count, options, metadata, correlationId, skipVersion, skipMetric)

IndexRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.index, options, metadata, correlationId, skipVersion, skipMetric)

DropCollectionRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.dropCollection, options, metadata, correlationId, skipVersion, skipMetric)

DropIndexRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.dropIndex, options, metadata, correlationId, skipVersion, skipMetric)

BulkRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.bulk, options, metadata, correlationId, skipVersion, skipMetric)

PipelineRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.pipeline, options, metadata, correlationId, skipVersion, skipMetric)

TransactionRequest(;database::String, collection::String, document::Document,
    options::Document = Document(), metadata::Document = Document(),
    correlationId::String = "", skipVersion::Bool = false, skipMetric::Bool = false ) =
    Request(database, collection, document, Actions.transaction, options, metadata, correlationId, skipVersion, skipMetric)

function to_bson(r::Request, application::String)::Vector{UInt8}
    buf = Vector{UInt8}()
    d = Dict("database" => r.database, "collection" => r.collection,
        "document" => r.document, "application" => application)
    if r.action == Actions.create
        d["action"] = "create"
    elseif r.action == Actions.retrieve
        d["action"] = "retrieve"
    elseif r.action == Actions.update
        d["action"] = "update"
    elseif r.action == Actions.delete
        d["action"] = "delete"
    elseif r.action == Actions.count
        d["action"] = "count"
    elseif r.action == Actions.index
        d["action"] = "index"
    elseif r.action == Actions.dropCollection
        d["action"] = "dropCollection"
    elseif r.action == Actions.dropIndex
        d["action"] = "dropIndex"
    elseif r.action == Actions.bulk
        d["action"] = "bulk"
    elseif r.action == Actions.pipeline
        d["action"] = "pipeline"
    elseif r.action == Actions.transaction
        d["action"] = "transaction"
    end

    if length(r.options) > 0 d["options"] = r.options end
    if length(r.metadata) > 0 d["metadata"] = r.metadata end
    if length(r.correlationId) > 0 d["correlationId"] = r.correlationId end
    if r.skipVersion d["skipVersion"] = true end
    if r.skipMetric d["skipMetric"] = true end

    bson_write(buf, d)
    return buf
end
