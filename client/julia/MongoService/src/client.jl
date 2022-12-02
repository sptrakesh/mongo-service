import Sockets: connect, TCPSocket

struct Client
    host::String
    port::UInt16
    application::String
    c::TCPSocket

    function Client(h::String, p::UInt16, a::String)
        desc = isempty(h) || isequal(h, "localhost") ? connect(p) : connect(h, p)
        c = new(isempty(h) ? "localhost" : h, p, a, desc)
        @info "Connected to $(c.host):$p"
        c
    end
end

Client(a::String) = Client("localhost", 2000, a)
Client(p::Integer, a::String) = Client("localhost", UInt16(p), a)

Base.isopen(c::Client) = isopen(c.c)

Base.close(c::Client) =
begin
    isopen(c.c) && close(c.c)
    @info "Closed connection to $(c.host):$(c.port)"
end
