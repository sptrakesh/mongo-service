//
// Created by Rakesh on 08/01/2020.
//

#include "bson.h"
#include "log/NanoLog.h"

#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/array/view.hpp>

#include <sstream>

namespace spt::util
{
  template<>
  bool bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_bool == type ) return view[key].get_bool().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to bool";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<bool> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bool>( key, view );
  }

  template<>
  int32_t bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_int32 == type ) return view[key].get_int32().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to int32";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<int32_t> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<int32_t>( key, view );
  }

  template<>
  int64_t bsonValue( std::string_view key, const bsoncxx::document::view& view )
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

      std::ostringstream ss;
      ss << "Invalid type for " << key;
      throw std::runtime_error( ss.str() );
    }
  }

  template<>
  std::optional<int64_t> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<int64_t>( key, view );
  }

  template<>
  double bsonValue( std::string_view key, const bsoncxx::document::view& view )
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

      std::ostringstream ss;
      ss << "Invalid type for " << key;
      throw std::runtime_error( ss.str() );
    }
  }

  template<>
  std::optional<double> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<double>( key, view );
  }

  template<>
  std::string bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_utf8 == type )
    {
#ifdef __APPLE__
      const auto value = view[key].get_string().value;
#else
      const auto value = view[key].get_utf8().value;
#endif
      return std::string( value.data(), value.size() );
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<std::string> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::string>( key, view );
  }

  template<>
  bsoncxx::oid bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_oid == type ) return view[key].get_oid().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<bsoncxx::oid> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bsoncxx::oid>( key, view );
  }

  template<>
  std::chrono::time_point<std::chrono::system_clock> bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::time_point<std::chrono::system_clock>( view[key].get_date().value );

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<std::chrono::time_point<std::chrono::system_clock>> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::chrono::time_point<std::chrono::system_clock>>( key, view );
  }

  template<>
  std::chrono::milliseconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return view[key].get_date().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to milliseconds";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<std::chrono::milliseconds> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<std::chrono::milliseconds>( key, view );
  }

  template<>
  bsoncxx::document::view bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_document == type ) return view[key].get_document().view();

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to document";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<bsoncxx::document::view> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bsoncxx::document::view>( key, view );
  }

  template<>
  bsoncxx::array::view bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_array == type ) return view[key].get_array().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to array";

    std::ostringstream ss;
    ss << "Invalid type for " << key;
    throw std::runtime_error( ss.str() );
  }

  template<>
  std::optional<bsoncxx::array::view> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return std::nullopt;
    return bsonValue<bsoncxx::array::view>( key, view );
  }

  std::string toString( std::string_view key, const bsoncxx::document::view& view )
  {
    auto it = view.find( key );
    if ( it == view.end() ) return {};

    std::ostringstream ss;
    switch ( it->type() )
    {
    case bsoncxx::type::k_bool:
      ss << std::boolalpha << it->get_bool();
      break;
    case bsoncxx::type::k_int32:
      ss << it->get_int32();
      break;
    case bsoncxx::type::k_int64:
      ss << it->get_int64();
      break;
    case bsoncxx::type::k_double:
      ss << it->get_double();
      break;
    case bsoncxx::type::k_date:
      ss << bsonValue<std::chrono::milliseconds>( key, view ).count();
      break;
    case bsoncxx::type::k_oid:
      ss << it->get_oid().value.to_string();
      break;
    case bsoncxx::type::k_utf8:
      return bsonValue<std::string>( key, view );
    default:
      ss << "Unknown type";
    }

    return ss.str();
  }
}
