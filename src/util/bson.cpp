//
// Created by Rakesh on 08/01/2020.
//

#include "bson.h"

#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>

namespace spt::util
{
  template<>
  bool bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_bool == type ) return view[key].get_bool().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to bool";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<bool> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bool>( key, view );
  }

  template<>
  int32_t bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_int32 == type ) return view[key].get_int32().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to int32";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<int32_t> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<int32_t>( key, view );
  }

  template<>
  int64_t bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    switch (type)
    {
    case bsoncxx::type::k_int32:
      return view[key].get_int32().value;
    case bsoncxx::type::k_int64:
      return view[key].get_int64().value;
    default:
      LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to int64";
      throw std::runtime_error( "Invalid type for " + key );
    }
  }

  template<>
  std::optional<int64_t> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<int64_t>( key, view );
  }

  template<>
  double bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    switch (type)
    {
    case bsoncxx::type::k_int32:
      return view[key].get_int32().value;
    case bsoncxx::type::k_int64:
      return view[key].get_int64().value;
    case bsoncxx::type::k_double:
      return view[key].get_double().value;
    default:
      LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to double";
      throw std::runtime_error( "Invalid type for " + key );
    }
  }

  template<>
  std::optional<double> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<double>( key, view );
  }

  template<>
  std::string bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_utf8 == type )
    {
      const auto value = view[key].get_utf8().value;
      return std::string( value.data(), value.size() );
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<std::string> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::string>( key, view );
  }

  template<>
  bsoncxx::oid bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_oid == type ) return view[key].get_oid().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<bsoncxx::oid> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bsoncxx::oid>( key, view );
  }

  template<>
  std::chrono::time_point<std::chrono::system_clock> bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::time_point<std::chrono::system_clock>( view[key].get_date().value );

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<std::chrono::time_point<std::chrono::system_clock>> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::chrono::time_point<std::chrono::system_clock>>( key, view );
  }

  template<>
  std::chrono::milliseconds bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return view[key].get_date().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to milliseconds";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<std::chrono::milliseconds> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::chrono::milliseconds>( key, view );
  }

  template<>
  bsoncxx::document::view bsonValue( const std::string& key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_document == type ) return view[key].get_document().view();

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to document";
    throw std::runtime_error( "Invalid type for " + key );
  }

  template<>
  std::optional<bsoncxx::document::view> bsonValueIfExists( const std::string& key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bsoncxx::document::view>( key, view );
  }
}

