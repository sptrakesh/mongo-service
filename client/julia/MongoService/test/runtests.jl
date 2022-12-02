using MongoService
using Test

@testset "MongoService.jl" begin
    @testset "Connection tests" begin
        c = Client(2000, "julia-test")
        close(c)
    end

    include("crud.jl")
    include("index.jl")
    include("bulk.jl")
end
