//
// Created by Rakesh on 18/07/2020.
//

#pragma once

#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <vector>

namespace spt::server
{
  struct Session : std::enable_shared_from_this<Session>
  {
    explicit Session( boost::asio::ip::tcp::socket socket );

    void start();

  private:
    void doRead();
    void doWrite( std::size_t length );

    boost::asio::ip::tcp::socket socket;
    std::vector<char> data;
  };
}
