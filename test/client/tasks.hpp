//
// Created by Rakesh on 15/07/2021.
//

#pragma once
#include "client.hpp"

#include <boost/asio/awaitable.hpp>

namespace spt::client
{
  boost::asio::awaitable<void> crud( Client& client );
  boost::asio::awaitable<void> crud();
  boost::asio::awaitable<void> apicrud();
}