//
// Created by Rakesh on 08/01/2020.
//

#include "bson.hpp"
#include "date.hpp"
#if defined __has_include
  #if __has_include("../../log/NanoLog.hpp")
    #include "../../log/NanoLog.hpp"
  #else
    #include <log/NanoLog.h>
  #endif
#endif

#include <boost/json/serialize.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/exception/exception.hpp>

#include <format>
#include <iomanip>

namespace spt::util
{
  template<>
  bool bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_bool == type ) return view[key].get_bool().value;

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to bool";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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

      throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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

      throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    if ( bsoncxx::type::k_string == type )
    {
      const auto value = view[key].get_string().value;
      return { value.data(), value.size() };
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string";

    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    if ( bsoncxx::type::k_string == type )
    {
      return view[key].get_string().value;
    }

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to string_view";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
  }

  template<>
  std::optional<bsoncxx::oid> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<bsoncxx::oid>( key, view );
  }

  template<>
  DateTime bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    using enum bsoncxx::type;
    const auto type = view[key].type();
    if ( k_date == type ) return DateTime{ view[key].get_date().value };
    if ( k_int32 == type ) return DateTime{ std::chrono::seconds{ view[key].get_int32().value } };
    if ( k_int64 == type ) return DateTime{ std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::microseconds{ view[key].get_int64().value } ) };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
  }

  template<>
  std::optional<DateTime> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<DateTime>( key, view );
  }

  template<>
  DateTimeMs bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    using enum bsoncxx::type;
    const auto type = view[key].type();
    if ( k_date == type ) return DateTimeMs{ view[key].get_date().value };
    if ( k_int32 == type ) return DateTimeMs{ std::chrono::seconds{ view[key].get_int32().value } };
    if ( k_int64 == type ) return DateTimeMs{ std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::microseconds{ view[key].get_int64().value } ) };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
  }

  template<>
  std::optional<DateTimeMs> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<DateTimeMs>( key, view );
  }

  template<>
  DateTimeNs bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    using enum bsoncxx::type;
    const auto type = view[key].type();
    if ( k_date == type ) return DateTimeNs{ view[key].get_date().value };
    if ( k_int32 == type ) return DateTimeNs{ std::chrono::seconds{ view[key].get_int32().value } };
    if ( k_int64 == type ) return DateTimeNs{ std::chrono::microseconds{ view[key].get_int64().value } };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to date";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
  }

  template<>
  std::optional<DateTimeNs> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<DateTimeNs>( key, view );
  }

  template<>
  std::chrono::seconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return std::chrono::duration_cast<std::chrono::seconds>( view[key].get_date().value );
    if ( bsoncxx::type::k_int32 == type ) return std::chrono::seconds{ view[key].get_int32().value };
    if ( bsoncxx::type::k_int64 == type ) return std::chrono::seconds{ view[key].get_int64().value };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to milliseconds";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
  }

  template<>
  std::optional<std::chrono::seconds> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view )
  {
    if ( auto it = view.find( key ); it == view.end() || it->type() == bsoncxx::type::k_null ) return std::nullopt;
    return bsonValue<std::chrono::seconds>( key, view );
  }

  template<>
  std::chrono::milliseconds bsonValue( std::string_view key, const bsoncxx::document::view& view )
  {
    const auto type = view[key].type();
    if ( bsoncxx::type::k_date == type ) return view[key].get_date().value;
    if ( bsoncxx::type::k_int32 == type ) return std::chrono::milliseconds{ view[key].get_int32().value };
    if ( bsoncxx::type::k_int64 == type ) return std::chrono::milliseconds{ view[key].get_int64().value };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to milliseconds";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    if ( bsoncxx::type::k_int32 == type ) return std::chrono::microseconds{ view[key].get_int32() };
    if ( bsoncxx::type::k_int64 == type ) return std::chrono::microseconds{ view[key].get_int64() };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to microseconds";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    if ( bsoncxx::type::k_int32 == type ) return std::chrono::nanoseconds{ view[key].get_int32() };
    if ( bsoncxx::type::k_int64 == type ) return std::chrono::nanoseconds{ view[key].get_int64() };

    LOG_WARN << "Key: " << key << " type: " << bsoncxx::to_string( type ) << " not convertible to nanoseconds";
    throw std::runtime_error( std::format( "Invalid type for {}", key ) );
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
    case bsoncxx::type::k_string:
      return bsonValue<std::string>( key, view );
    default:
      return "Unknown type"s;
    }

    return {};
  }

  namespace pbson
  {
    std::variant<std::optional<std::string>, bsoncxx::oid, DateTime> parseString( const boost::json::value& value )
    {
      using O =  std::variant<std::optional<std::string>, bsoncxx::oid, DateTime>;
      if ( !value.is_string() )
      {
        LOG_CRIT << "Value not string";
        return O{ std::nullopt };
      }

      const auto& v = value.as_string();
      if ( v.empty() ) return std::nullopt;

      if ( v.size() == 24 && v.find( '-' ) == boost::json::string::npos )
      {
        try
        {
          LOG_INFO << "Attempting to parse value " << v << " as BSON ObjectId.";
          return O{ bsoncxx::oid{ v } };
        }
        catch ( const bsoncxx::exception& ex )
        {
          LOG_DEBUG << "Error trying to parse " << v << " as potential BSON ObjectId";
        }
      }

      if ( v.front() > 47 && v.front() < 58 && v.find( '-' ) != boost::json::string::npos )
      {
        LOG_INFO << "Attempting to parse value " << v << " as ISO8601 date-time.";
        auto dt = parseISO8601( v );
        if ( dt.has_value() ) return O{ dt.value() };
      }

      return O{ std::string{ v } };
    }
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
      arr.emplace_back( isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( e.get_date().value ) ) );
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
    case bsoncxx::type::k_string:
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
      root.emplace( key, isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( e.get_date().value ) ) );
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
    case bsoncxx::type::k_string:
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

bsoncxx::array::value spt::util::toBson( const boost::json::array& array )
{
  using boost::json::kind;
  auto arr = bsoncxx::builder::stream::array{};

  for ( const auto& item : array )
  {
    switch ( item.kind() )
    {
    case kind::bool_:
      arr << item.as_bool();
      break;
    case kind::array:
      arr << toBson( item.as_array() );
      break;
    case kind::object:
      arr << toBson( item.as_object() );
      break;
    case kind::string:
    {
      auto parsed = pbson::parseString( item );
      if ( std::holds_alternative<bsoncxx::oid>( parsed ) ) arr << std::get<bsoncxx::oid>( parsed );
      else if ( std::holds_alternative<DateTime>( parsed ) ) arr << bsoncxx::types::b_date{ std::get<DateTime>( parsed ) };
      else
      {
        auto str = std::get<std::optional<std::string>>( parsed );
        if ( str ) arr << *str;
      }
      break;
    }
    case kind::double_:
      arr << item.as_double();
      break;
    case kind::int64:
      arr << item.as_int64();
      break;
    case kind::uint64:
      arr << static_cast<int64_t>( item.as_uint64() );
      break;
    case kind::null:
      break;
    }
  }

  return arr << bsoncxx::builder::stream::finalize;
}

bsoncxx::document::value spt::util::toBson( const boost::json::object& object )
{
  using boost::json::kind;
  auto obj = bsoncxx::builder::stream::document{};

  for ( const auto& [key,value] : object )
  {
    switch ( value.kind() )
    {
    case kind::bool_:
      obj << std::string_view{ key } << value.as_bool();
      break;
    case kind::array:
      obj << std::string_view{ key } << toBson( value.as_array() );
      break;
    case kind::object:
      obj << std::string_view{ key } << toBson( value.as_object() );
      break;
    case kind::string:
    {
      auto parsed = pbson::parseString( value );
      if ( std::holds_alternative<bsoncxx::oid>( parsed ) ) obj << std::string_view{ key } << std::get<bsoncxx::oid>( parsed );
      else if ( std::holds_alternative<DateTime>( parsed ) ) obj << std::string_view{ key } << bsoncxx::types::b_date{ std::get<DateTime>( parsed ) };
      else
      {
        auto str = std::get<std::optional<std::string>>( parsed );
        if ( str ) obj << std::string_view{ key } << *str;
      }
      break;
    }
    case kind::double_:
      obj << std::string_view{ key } << value.as_double();
      break;
    case kind::int64:
      obj << std::string_view{ key } << value.as_int64();
      break;
    case kind::uint64:
      obj << std::string_view{ key } << static_cast<int64_t>( value.as_uint64() );
      break;
    case kind::null:
      break;
    }
  }

  return obj << bsoncxx::builder::stream::finalize;
}

boost::json::array spt::util::fromBson( bsoncxx::array::view array )
{
  using bsoncxx::type;
  auto arr = boost::json::array{};

  for ( const auto& item : array )
  {
    switch ( item.type() )
    {
    case type::k_bool:
      arr.emplace_back( item.get_bool().value );
      break;
    case type::k_int32:
      arr.emplace_back( item.get_int32().value );
      break;
    case type::k_int64:
      arr.emplace_back( item.get_int64().value );
      break;
    case type::k_double:
      arr.emplace_back( item.get_double().value );
      break;
    case type::k_oid:
      arr.emplace_back( item.get_oid().value.to_string() );
      break;
    case type::k_date:
      arr.emplace_back( isoDateMillis( item.get_date().value ) );
      break;
    case type::k_string:
      arr.emplace_back( item.get_string().value );
      break;
    case type::k_array:
      arr.emplace_back( fromBson( item.get_array().value ) );
      break;
    case type::k_document:
      arr.emplace_back( fromBson( item.get_document().value ) );
      break;
    default:
      break;
    }
  }

  return arr;
}

boost::json::object spt::util::fromBson( bsoncxx::document::view document )
{
  using bsoncxx::type;
  auto obj = boost::json::object{};

  for ( const auto& item : document )
  {
    switch ( item.type() )
    {
    case type::k_bool:
      obj.emplace( std::string_view{ item.key() }, item.get_bool().value );
      break;
    case type::k_int32:
      obj.emplace( std::string_view{ item.key() }, item.get_int32().value );
      break;
    case type::k_int64:
      obj.emplace( std::string_view{ item.key() }, item.get_int64().value );
      break;
    case type::k_double:
      obj.emplace( std::string_view{ item.key() }, item.get_double().value );
      break;
    case type::k_oid:
      obj.emplace( std::string_view{ item.key() }, item.get_oid().value.to_string() );
      break;
    case type::k_date:
      obj.emplace( std::string_view{ item.key() }, isoDateMillis( item.get_date().value ) );
      break;
    case type::k_string:
      obj.emplace( std::string_view{ item.key() }, item.get_string().value );
      break;
    case type::k_array:
      obj.emplace( std::string_view{ item.key() }, fromBson( item.get_array().value ) );
      break;
    case type::k_document:
      obj.emplace( std::string_view{ item.key() }, fromBson( item.get_document().value ) );
      break;
    default:
      break;
    }
  }

  return obj;
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

bsoncxx::oid spt::util::generateId( std::chrono::system_clock::time_point timestamp, bsoncxx::oid id )
{
  char bytes[12];
  [[maybe_unused]] auto b = id.bytes();
  std::memcpy( bytes, id.bytes(), 12 );

  const auto swapEndian = []( const int32_t source, int32_t& dest )
  {
    const char* in = reinterpret_cast<const char*>( &source );
    char* out = reinterpret_cast<char*>( &dest );

#if BSON_BYTE_ORDER == BSON_BIG_ENDIAN
    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];
#else
    for ( int i = 0; i < 4; ++i ) out[i] = in[i];
#endif
  };

  int32_t ts{ 0 };
  swapEndian( static_cast<int32_t>( std::chrono::duration_cast<std::chrono::seconds>( timestamp.time_since_epoch() ).count() ), ts );
  char* out = reinterpret_cast<char*>( &ts );

  for ( int i = 0; i < 4; ++i ) bytes[i] = out[i];
  return bsoncxx::oid{ bytes, 12 };
}
