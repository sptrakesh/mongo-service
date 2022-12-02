import MongoService: Client, BulkRequest, CountRequest, Document, execute
import LightBSON: BSONObjectId

@testset "Bulk operation test suite" begin
    c = Client(2000, "julia-test")
    id1 = BSONObjectId()
    id2 = BSONObjectId()
    ids = Vector{BSONObjectId}()
    db = "itest"
    col = "test"
    count = 0

    @testset "Creating documents in bulk" begin
        doc = Document("insert" =>
            [Document("_id" => id1, "key" => "value1"),
            Document("_id" => id2, "key" => "value2")])
        req = BulkRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Bulk: create: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "create")
        @test haskey(resp, "history")
        @test haskey(resp, "delete")
    end

    @testset "Retriving count of documents" begin
        req = CountRequest(database=db, collection=col)
        resp = execute(c, req)
        @info "Bulk: count: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "count")
        count = resp["count"]
    end

    @testset "Deleting documents in bulk" begin
        doc = Document("delete" => [Document("_id" => id1), Document("_id" => id2)])
        req = BulkRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Bulk: delete: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "create")
        @test haskey(resp, "history")
        @test haskey(resp, "delete")
    end

    @testset "Retriving count of documents after delete" begin
        req = CountRequest(database=db, collection=col)
        resp = execute(c, req)
        @info "Bulk: count: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "count")
        @test count > resp["count"]
    end

    @testset "Creating a large batch of documents" begin
        docs = Vector{Document}()
        size = 10000
        for i in 1:size
            id = BSONObjectId()
            push!(ids, id)
            push!(docs, Document("_id" => id, "iter" => i,
                "key1" => "value1", "key2" => "value2", "key3" => "value3", "key4" => "value4", "key5" => "value5",
                "sub1" => Document("key1" => "value1", "key2" => "value2", "key3" => "value3", "key4" => "value4", "key5" => "value5"),
                "sub2" => Document("key1" => "value1", "key2" => "value2", "key3" => "value3", "key4" => "value4", "key5" => "value5"),
                "sub3" => Document("key1" => "value1", "key2" => "value2", "key3" => "value3", "key4" => "value4", "key5" => "value5")))
        end

        doc = Document("insert" => docs)
        req = BulkRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Bulk: create large batch: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "create")
        @test resp["create"] == size
        @test haskey(resp, "history")
        @test resp["history"] == size
        @test haskey(resp, "delete")
        @test resp["delete"] == 0
    end

    @testset "Deleting a large batch of documents" begin
        docs = Vector{Document}()
        for id in ids push!(docs, Document("_id" => id)) end

        doc = Document("delete" => docs)
        req = BulkRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Bulk: delete large batch: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "create")
        @test resp["create"] == 0
        @test haskey(resp, "history")
        @test resp["history"] == length(ids)
        @test haskey(resp, "delete")
        @test resp["delete"] == length(ids)
    end

    close(c)
end