//
// Created by Rakesh on 22/02/2022.
//

#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>

#include <memory>
#include <optional>

namespace spt::mongoservice::api::impl
{
  struct AsyncConnection
  {
    AsyncConnection( boost::asio::io_context& ioc, std::string_view host, std::string_view port );
    ~AsyncConnection();

    AsyncConnection( const AsyncConnection& ) = delete;
    AsyncConnection& operator=( const AsyncConnection& ) = delete;

    using Response = std::optional<bsoncxx::document::value>;
    [[nodiscard]] boost::asio::awaitable<Response> execute( bsoncxx::document::view view );

    [[nodiscard]] bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    [[nodiscard]] boost::asio::awaitable<bool> connect();

    boost::asio::ip::tcp::socket s;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    std::string host;
    std::string port;
    bool v{ true };
  };

  std::unique_ptr<AsyncConnection> createAsyncConnection();
}