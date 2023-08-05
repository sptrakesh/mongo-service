//
// Created by Rakesh on 20/07/2020.
//

#pragma once
#include "model/document.h"

#include <boost/asio/awaitable.hpp>
#include <bsoncxx/document/view_or_value.hpp>

namespace spt::db
{
  boost::asio::awaitable<bsoncxx::document::view_or_value> process( const spt::model::Document& document );
}
