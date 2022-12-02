# MongoService
Sample [Julia](https://julialang.org) client implementation for the service.  The [Client](src/client.jl) handles
the TCP/IP connection to the service.  A [Request](src/request.jl) structure similar to the C++ API is available
to handle requests made to the service.  A single `execute` function is implemented in the [module](src/MongoService.jl)
which sends the request to the service, and returns the response as a `OrderedDict{String,Any}` (the default used
by the LightBSON package.

See [integration](test/runtests.jl) test suite for sample use of the package.

## Set up

Install the client as a `dev` package.  Also install the [LightBSON](https://github.com/ancapdev/LightBSON.jl) dependency.

Start the Julia Pkg REPL.

```julia
julia> ]
pkg> add LightBSON
pkg> dev <path to>/mongo-service/client/julia/MongoService
pkg> [delete]
julia> using Pkg
julia> Pkg.test("MongoService")
```
