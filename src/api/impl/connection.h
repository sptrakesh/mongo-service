//
// Created by Rakesh on 22/09/2020.
//

#pragma once

#include <optional>
#include <string_view>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>

using tcp = boost::asio::ip::tcp;

namespace spt::mongoservice::api::impl
{
  struct Connection
  {
    Connection( boost::asio::io_context& ioc, std::string_view host, std::string_view port );

    Connection( const Connection& ) = delete;
    Connection& operator=( const Connection& ) = delete;

    ~Connection()
    {
      s.close();
    }

    [[nodiscard]] std::optional<bsoncxx::document::value> execute(
        bsoncxx::document::view document, std::size_t bufSize = 4 * 1024 );

    [[nodiscard]] bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    tcp::socket& socket();

    tcp::socket s;
    tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    boost::asio::streambuf buffer;
    std::string host;
    std::string port;
    bool v{ true };
  };

  std::unique_ptr<Connection> create();
}
