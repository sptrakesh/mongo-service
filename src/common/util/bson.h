//
// Created by Rakesh on 08/01/2020.
//

#pragma once

#include <optional>
#include <ostream>

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/oid.hpp>

namespace spt::util
{
  template<typename DataType>
  DataType bsonValue( std::string_view key, const bsoncxx::document::view& view );

  template<typename DataType>
  std::optional<DataType> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view );

  std::string toString( std::string_view key, const bsoncxx::document::view& view );

  boost::json::array toJson( const bsoncxx::array::view& view );
  std::ostream& toJson( std::ostream& os, const bsoncxx::array::view& view );
  boost::json::object toJson( const bsoncxx::document::view& view );
  std::ostream& toJson( std::ostream& os, const bsoncxx::document::view& view );

  bsoncxx::array::value toBson( const boost::json::array& array );
  bsoncxx::document::value toBson( const boost::json::object& object );

  boost::json::array fromBson( bsoncxx::array::view array );
  boost::json::object fromBson( bsoncxx::document::view object );

  std::optional<bsoncxx::oid> parseId( std::string_view id );
}
