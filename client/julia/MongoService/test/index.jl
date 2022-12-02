import MongoService: Client, IndexRequest, DropIndexRequest, execute

@testset "Index test suite" begin
    c = Client(2000, "julia-test")
    db = "itest"
    col = "test"

    @testset "Creating an index" begin
        doc = Document("unused" => 1)
        req = IndexRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Index: creating index: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "name")
    end

    @testset "Creating the index again" begin
        doc = Document("unused" => 1)
        req = IndexRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Index: creating index again: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "name") == false
    end

    @testset "Dropping the index" begin
        doc = Document("specification" => Document("unused" => 1))
        req = DropIndexRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Index: dropping index: $resp"

        @test haskey(resp, "error") == false
    end

    @testset "Creating a unique index" begin
        doc = Document("unused" => 1)
        opts = Document("name" => "uniqueIndex", "unique" => true, "expireAfterSeconds" => 5)
        req = IndexRequest(database=db, collection=col, document=doc, options=opts)
        resp = execute(c, req)
        @info "Index: creating unique index: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "name")
    end

    @testset "Creating a unique index again" begin
        doc = Document("unused" => 1)
        opts = Document("name" => "uniqueIndex", "unique" => true, "expireAfterSeconds" => 5)
        req = IndexRequest(database=db, collection=col, document=doc, options=opts)
        resp = execute(c, req)
        @info "Index: creating unique index again: $resp"

        @test haskey(resp, "error") == false
        @test haskey(resp, "name") == false
    end

    @testset "Dropping the unique index" begin
        doc = Document("name" => "uniqueIndex")
        req = DropIndexRequest(database=db, collection=col, document=doc)
        resp = execute(c, req)
        @info "Index: dropping unique index: $resp"

        @test haskey(resp, "error") == false
    end

end