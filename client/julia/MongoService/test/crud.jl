import MongoService: Client, CreateRequest, UpdateRequest, RetrieveRequest, DeleteRequest, Document, execute
import LightBSON: BSONObjectId

@testset "CRUD test suite" begin
    c = Client(2000, "julia-test")
    id = BSONObjectId()
    db = "itest"
    col = "test"
    count = 0

    vhdb = ""
    vhc = ""
    vhoid = BSONObjectId()

    @testset "Create a document" begin
        doc = Document("_id" => id, "key" => "value")
        req = CreateRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "CRUD: create: $resp"

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

    @testset "Retrieve count of documents" begin
        req = CountRequest(database=db, collection=col, skipMetric=true)
        resp = execute(c, req)
        @info "CRUD: count: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "count")
        count = resp["count"]
    end

    @testset "Retrieving the document by id" begin
        doc = Document("_id" => id)
        req = RetrieveRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "CRUD: retrieve by id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result")
        @test haskey(resp, "results") == false
        @test haskey(resp["result"], "_id")
        @test resp["result"]["_id"] == id
        @test haskey(resp["result"], "key")
        @test resp["result"]["key"] == "value"
    end

    @testset "Retrieving the document by property" begin
        doc = Document("key" => "value")
        req = RetrieveRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "CRUD: retrieve by property: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result") == false
        @test haskey(resp, "results")

        found = false
        for d in resp["results"]
            @test haskey(d, "_id")
            if d["_id"] == id found = true end
        end
        @test found
    end

    @testset "Retrieve the version history document by id" begin
        doc = Document("_id" => vhoid)
        req = RetrieveRequest(database=vhdb, collection=vhc, document=doc)
        resp = execute(c, req)
        @info "CRUD: retrieve version history by id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result")
        @test haskey(resp, "results") == false
        @test haskey(resp["result"], "_id")
        @test resp["result"]["_id"] == vhoid
        @test haskey(resp["result"], "entity")
        @test haskey(resp["result"]["entity"], "_id")
        @test resp["result"]["entity"]["_id"] == id
    end

    @testset "Retrieve the version history document by entity id" begin
        doc = Document("entity._id" => id)
        req = RetrieveRequest(database=vhdb, collection=vhc, document=doc)
        resp = execute(c, req)
        @info "CRUD: retrieve version history by entity id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "result") == false
        @test haskey(resp, "results")
        @test length(resp["results"]) > 0
        @test haskey(resp["results"][1], "_id")
        @test resp["results"][1]["_id"] == vhoid
        @test haskey(resp["results"][1], "entity")
        @test haskey(resp["results"][1]["entity"], "_id")
        @test resp["results"][1]["entity"]["_id"] == id
    end

    @testset "Updating the document by id" begin
        doc = Document("_id" => id, "key1" => "value1")
        req = UpdateRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "CRUD: updating document by id: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "skipVersion") == false
        @test haskey(resp, "document")
        @test haskey(resp["document"], "_id")
        @test resp["document"]["_id"] == id
        @test haskey(resp["document"], "key")
        @test resp["document"]["key"] == "value"
        @test haskey(resp["document"], "key1")
        @test resp["document"]["key1"] == "value1"
    end

    @testset "Updating the document without version history" begin
        doc = Document("_id" => id, "key1" => "value1")
        req = UpdateRequest(database=db, collection=col, document=doc, skipVersion=true)
        resp = execute(c, req)
        @info "CRUD: updating document without version history: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "skipVersion")
        @test resp["skipVersion"]
    end

    @testset "Deleting the document" begin
        doc = Document("_id" => id)
        req = DeleteRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "CRUD: deleting document: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "success")
        @test haskey(resp, "history")
    end

    @testset "Retriving count of documents after delete" begin
        req = CountRequest(database=db, collection=col)
        resp = execute(c, req)
        @info "CRUD: count: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "count")
        @test resp["count"] < count
    end

    close(c)
end
