import MongoService: Client, CreateRequest, DropCollectionRequest, RenameCollectionRequest, RetrieveRequest, Document, execute
import LightBSON: BSONObjectId

@testset "Rename collection test suite" begin
    c = Client(2000, "julia-test")
    id = BSONObjectId()
    db = "itest"
    col = "oldname"
    target = "newname"
    count = 0

    vhdb = ""
    vhc = ""
    vhoid = BSONObjectId()

    @testset "Document created in a new collection" begin
        doc = Document("_id" => id, "key" => "value")
        req = CreateRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Rename: create: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "database")
        @test haskey(resp, "collection")
        @test haskey(resp, "_id")
        @test haskey(resp, "entity")
        @test resp["entity"] == id

        vhdb = resp["database"]
        vhc = resp["collection"]
        vhoid = resp["_id"]
    end

    @testset "Cannot rename to existing collection" begin
        doc = Document("target" => "test")
        req = RenameCollectionRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Rename: existing: $resp"

        @test haskey(resp, "error")
    end

    @testset "Rename collection" begin
        doc = Document("target" => target)
        req = RenameCollectionRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Rename: rename: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "database")
        @test haskey(resp, "collection")
        @test resp["database"] == db
        @test resp["collection"] == target
    end

    @testset "Retrieve document from renamed collection" begin
        doc = Document("_id" => id)
        req = RetrieveRequest(database=db, collection=target, document=doc)
        resp = execute(c, req)
        @info "Rename: retrieve by id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result")
        @test haskey(resp, "results") == false
        @test haskey(resp["result"], "_id")
        @test resp["result"]["_id"] == id
        @test haskey(resp["result"], "key")
        @test resp["result"]["key"] == "value"
    end

    @testset "Retrieve version history for renamed collection" begin
        doc = Document("_id" => vhoid, "database" => db, "collection" => target)
        req = RetrieveRequest(database=vhdb, collection=vhc, document=doc)
        resp = execute(c, req)
        @info "Rename: retrieve version history by id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result")
        @test haskey(resp, "results") == false
        @test haskey(resp["result"], "_id")
        @test resp["result"]["_id"] == vhoid
        @test haskey(resp["result"], "entity")
        @test haskey(resp["result"]["entity"], "_id")
        @test resp["result"]["entity"]["_id"] == id
    end

    @testset "Drop renamed collection" begin
        doc = Document("clearVersionHistory" => true)
        req = DropCollectionRequest(database=db, collection=target, document=doc)
        resp = execute(c, req)
        @info "Rename: drop collection: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "dropCollection")
    end

    @testset "Revision history cleared for dropped collection" begin
        sleep(0.25)
        doc = Document("_id" => vhoid, "database" => db, "collection" => target)
        req = RetrieveRequest(database=vhdb, collection=vhc, document=doc)
        resp = execute(c, req)
        @info "Rename: retrieve version history by id after clear: $resp"

        @test haskey(resp, "error")
    end
end
