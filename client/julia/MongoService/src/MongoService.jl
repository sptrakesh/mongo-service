module MongoService
include("client.jl")
include("request.jl")

import LightBSON: bson_read

export Client, Request, execute
export CreateRequest, RetrieveRequest, UpdateRequest, DeleteRequest, CountRequest
export IndexRequest, DropCollectionRequest, DropIndexRequest
export BulkRequest, PipelineRequest, TransactionRequest

function execute(c::Client, request::Request)
    bytes = to_bson(request, c.application)
    @info "execute: Writing $(length(bytes)) bytes to socket"
    s = write(c.c, bytes)
    @info "execute: Wrote $s payload bytes to socket"
    flush(c.c)

    @info "execute: Reading response size into array of 4 bytes"
    lv = read(c.c, 4)
    l = reinterpret(UInt32, lv)
    @info "execute: Response size: $(l[1])"

    b = read(c.c, l[1]-4)
    bytes = vcat(lv, b)
    @info "execute: Read response of size $(length(bytes)) bytes"
    bson_read(bytes)
end

end