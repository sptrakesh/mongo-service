//
// Created by Rakesh on 15/07/2021.
//

#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/document/value.hpp>

#include <memory>
#include <optional>

namespace spt::client
{
  struct Client
  {
    Client( boost::asio::io_context& ioc, std::string_view h,
        std::string_view p, boost::system::error_code& ec );
    ~Client();

    Client( const Client& ) = delete;
    Client& operator=( const Client& ) = delete;

    [[nodiscard]] boost::asio::awaitable<std::optional<bsoncxx::document::value>> execute(
        bsoncxx::document::view view );

    [[nodiscard]] bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    boost::asio::ip::tcp::socket s;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    std::string host;
    std::string port;
    bool v{ true };
  };

  std::unique_ptr<Client> createClient();
}