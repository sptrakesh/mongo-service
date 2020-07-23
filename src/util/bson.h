//
// Created by Rakesh on 08/01/2020.
//

#pragma once

#include <optional>
#include <bsoncxx/document/view.hpp>

namespace spt::util
{
  template<typename DataType>
  DataType bsonValue( std::string_view key, const bsoncxx::document::view& view );

  template<typename DataType>
  std::optional<DataType> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view );

  std::string toString( std::string_view key, const bsoncxx::document::view& view );
}
