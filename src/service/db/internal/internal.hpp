//
// Created by Rakesh on 05/08/2023.
//

#pragma once
#include "model/document.hpp"

#include <boost/asio/awaitable.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <mongocxx/write_concern.hpp>

namespace spt::db::internal
{
  mongocxx::write_concern writeConcern( bsoncxx::document::view view );
  boost::asio::awaitable<bsoncxx::document::view_or_value> dropCollection( const model::Document& model );
  boost::asio::awaitable<bsoncxx::document::view_or_value> transaction( const spt::model::Document& document );
  boost::asio::awaitable<bsoncxx::document::view_or_value> createCollection( const spt::model::Document& document );
  boost::asio::awaitable<bsoncxx::document::view_or_value> renameCollection( const spt::model::Document& document );
}
