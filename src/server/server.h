//
// Created by Rakesh on 18/07/2020.
//

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace spt::server
{
  struct Server
  {
    explicit Server( boost::asio::io_context& ioc );

  private:
    void doAccept();

    boost::asio::ip::tcp::acceptor acceptor;
  };
}
