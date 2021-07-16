//
// Created by Rakesh on 15/07/2021.
//

#pragma once
#include "client.h"

#include <boost/asio/awaitable.hpp>

namespace spt::client
{
  boost::asio::awaitable<void> crud( Client& client );
}