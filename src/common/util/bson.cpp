//
// Created by Rakesh on 08/01/2020.
//

#include "bson.h"
#include "date.h"
#if defined __has_include
  #if __has_include("../../log/NanoLog.h")
    #include "../../log/NanoLog.h"
  #else
    #include <log/NanoLog.h>
  #endif
#endif

#include <boost/json/serialize.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/array/view.hpp>

#include <iomanip>

namespace spt::util
{
  template<>
  bool bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_bool == type ) return view[key].get_bool().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to bool";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<bool> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<bool>( key, view );
  }

  template<>
  int32_t bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_int32 == type ) return view[key].get_int32().value;
    if ( bsoncxx::type::k_int64 == type ) return static_cast<int32_t>( view[key].get_int64().value );

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to int32";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<int32_t> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
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
    case bsoncxx::type::k_date:
      return view[key].get_date().value.count();
    default:
      LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to int64";

      std::string ss;
      ss.reserve( 64 );
      ss.append( "Invalid type for " ).append( key );
      throw std::runtime_error( ss );
    }
  }

  template<>
  std::optional<int64_t> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
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

      std::string ss;
      ss.reserve( 64 );
      ss.append( "Invalid type for " ).append( key );
      throw std::runtime_error( ss );
    }
  }

  template<>
  std::optional<double> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<double>( key, view );
  }

  template<>
  std::string bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_utf8 == type )
    {
      const auto value = view[key].get_string().value;
      return { value.data(), value.size() };
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::string> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::string>( key, view );
  }

  template<>
  std::string_view bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_utf8 == type )
    {
      return view[key].get_string().value;
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string_view";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::string_view> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::string_view>( key, view );
  }

  template<>
  bsoncxx::oid bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_oid == type ) return view[key].get_oid().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<bsoncxx::oid> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<bsoncxx::oid>( key, view );
  }

  template<>
  std::chrono::time_point<std::chrono::system_clock> bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::time_point<std::chrono::system_clock>( view[key].get_date().value );

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::chrono::time_point<std::chrono::system_clock>> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::chrono::time_point<std::chrono::system_clock>>( key, view );
  }

  template<>
  std::chrono::milliseconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return view[key].get_date().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to milliseconds";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::chrono::milliseconds> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::chrono::milliseconds>( key, view );
  }

  template<>
  bsoncxx::document::view bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_document == type ) return view[key].get_document().view();

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to document";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<bsoncxx::document::view> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<bsoncxx::document::view>( key, view );
  }

  template<>
  bsoncxx::array::view bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_array == type ) return view[key].get_array().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to array";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<bsoncxx::array::view> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<bsoncxx::array::view>( key, view );
  }

  template<>
  std::chrono::microseconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::duration_cast<std::chrono::microseconds>( view[key].get_date().value );
    else if ( bsoncxx::type::k_int32 == type ) return std::chrono::microseconds{ view[key].get_int32() };
    else if ( bsoncxx::type::k_int64 == type ) return std::chrono::microseconds{ view[key].get_int64() };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to microseconds";

    std::string ss;
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::chrono::microseconds> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::chrono::microseconds>( key, view );
  }

  template<>
  std::chrono::nanoseconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::duration_cast<std::chrono::nanoseconds>( view[key].get_date().value );
    else if ( bsoncxx::type::k_int32 == type ) return std::chrono::nanoseconds{ view[key].get_int32() };
    else if ( bsoncxx::type::k_int64 == type ) return std::chrono::nanoseconds{ view[key].get_int64() };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to nanoseconds";

    std::string ss;
    ss.reserve( 64 );
    ss.append( "Invalid type for " ).append( key );
    throw std::runtime_error( ss );
  }

  template<>
  std::optional<std::chrono::nanoseconds> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::chrono::nanoseconds>( key, view );
  }

  std::string toString( std::string_view key, const bsoncxx::document::view& view )
  {
    using namespace std::string_literals;
    auto it = view.find( key );
    if ( it == view.end() ) return {};

    switch ( it->type() )
    {
    case bsoncxx::type::k_bool:
      return it->get_bool() ? "true"s : "false"s;
    case bsoncxx::type::k_int32:
      return std::to_string( it->get_int32() );
    case bsoncxx::type::k_int64:
      return std::to_string( it->get_int64() );
    case bsoncxx::type::k_double:
      return std::to_string( it->get_double() );
    case bsoncxx::type::k_date:
      return std::to_string( bsonValue<std::chrono::milliseconds>( key, view ).count() );
    case bsoncxx::type::k_oid:
      return it->get_oid().value.to_string();
    case bsoncxx::type::k_utf8:
      return bsonValue<std::string>( key, view );
    default:
      return "Unknown type"s;
    }

    return {};
  }
}

boost::json::array spt::util::toJson( const bsoncxx::array::view& view )
{
  auto arr = boost::json::array{};
  for ( auto&& e : view )
  {
    switch ( e.type() )
    {
    case bsoncxx::type::k_array:
      arr.emplace_back( toJson( e.get_array().value ) );
      break;
    case bsoncxx::type::k_document:
      arr.emplace_back( toJson( e.get_document().value ) );
      break;
    case bsoncxx::type::k_bool:
      arr.emplace_back( e.get_bool().value );
      break;
    case bsoncxx::type::k_date:
      arr.emplace_back( isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( e.get_date().value ).count() ) );
      break;
    case bsoncxx::type::k_double:
      arr.emplace_back( e.get_double().value );
      break;
    case bsoncxx::type::k_int32:
      arr.emplace_back( e.get_int32().value );
      break;
    case bsoncxx::type::k_int64:
      arr.emplace_back( e.get_int64().value );
      break;
    case bsoncxx::type::k_oid:
      arr.emplace_back( e.get_oid().value.to_string() );
      break;
    case bsoncxx::type::k_utf8:
    {
      auto v = e.get_string().value;
      arr.emplace_back( std::string_view{ v.data(), v.size() } );
      break;
    }
    default:
      // Not sure how to convert to JSON
      break;
    }
  }

  return arr;
}

std::ostream& spt::util::toJson( std::ostream& os, const bsoncxx::array::view& view )
{
  os << boost::json::serialize( toJson( view ) );
  return os;
}

boost::json::object spt::util::toJson( const bsoncxx::document::view& view )
{
  auto root = boost::json::object{};
  for ( auto&& e : view )
  {
    auto k = e.key();
    auto key = std::string_view{ k.data(), k.size() };

    switch ( e.type() )
    {
    case bsoncxx::type::k_array:
      root.emplace( key, toJson( e.get_array().value ) );
      break;
    case bsoncxx::type::k_document:
      root.emplace( key, toJson( e.get_document().value ) );
      break;
    case bsoncxx::type::k_bool:
      root.emplace( key, e.get_bool().value );
      break;
    case bsoncxx::type::k_date:
      root.emplace( key, isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( e.get_date().value ).count() ) );
      break;
    case bsoncxx::type::k_double:
      root.emplace( key, e.get_double().value );
      break;
    case bsoncxx::type::k_int32:
      root.emplace( key, e.get_int32().value );
      break;
    case bsoncxx::type::k_int64:
      root.emplace( key, e.get_int64().value );
      break;
    case bsoncxx::type::k_oid:
      root.emplace( key, e.get_oid().value.to_string() );
      break;
    case bsoncxx::type::k_utf8:
    {
      auto v = e.get_string().value;
      root.emplace( key, std::string_view{ v.data(), v.size() } );
      break;
    }
    default:
      // Not sure how to convert to JSON
      break;
    }
  }

  return root;
}

std::ostream& spt::util::toJson( std::ostream& os, const bsoncxx::document::view& view )
{
  os << toJson( view );
  return os;
}

std::optional<bsoncxx::oid> spt::util::parseId( std::string_view id )
{
  try
  {
    return bsoncxx::oid{ id };
  }
  catch ( const std::exception& ex )
  {
    LOG_WARN << "Unparseable id " << id << ". " << ex.what();
  }

  return std::nullopt;
}
