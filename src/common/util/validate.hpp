//
// Created by Rakesh on 21/02/2024.
//

#pragma once

#if defined __has_include
#if __has_include("../../log/NanoLog.hpp")
#include "../../log/NanoLog.hpp"
#else
#include <log/NanoLog.hpp>
#endif
#endif

#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/document/value.hpp>

namespace spt::util::json
{
  namespace impl
  {
    struct IgnoreList
    {
      static const IgnoreList& instance()
      {
        static IgnoreList il;
        return il;
      }

      IgnoreList(const IgnoreList&) = delete;
      IgnoreList& operator=(const IgnoreList&) = delete;
      IgnoreList(IgnoreList&&) = delete;
      IgnoreList& operator=(IgnoreList&&) = delete;

      std::vector<std::string> names{ "password", "version" };
      double ratio{ 0.4 };

    private:
      IgnoreList()
      {
        if ( const char* value = std::getenv( "SPT_JSON_PARSE_VALIDATION_IGNORE" ); value )
        {
          names.clear();
          names.reserve( 8 );
          auto str = std::string{ value };
          boost::algorithm::split( names, str, boost::is_any_of( " ," ) );
        }

        if ( const char* value = std::getenv( "SPT_JSON_PARSE_VALIDATION_RATIO" ); value )
        {
          try
          {
            ratio = boost::lexical_cast<double>( value );
          }
          catch ( const boost::bad_lexical_cast& e )
          {
            LOG_CRIT << "Invalid ratio specified " << value;
          }
        }
      }
    };

    bool validate( const char* name, std::string_view field );
    bool validate( const char* name, bsoncxx::array::view field );
    bool validate( const char* name, bsoncxx::document::view field );
    bool validate( const char* name, const boost::json::array& array );
    bool validate( const char* name, const boost::json::object& object );
  }

  /**
   * Validate the parsed JSON value in the specified field.  Generally only useful for string
   * types.  Implement your own version of this function as appropriate.
   *
   * The set functions invoke this function, and throw simdjson::simdjson_error exception when validation fails.
   * @tparam M The type of field that will be populated from the JSON value.
   * @param name The name of the field in the entity being de-serialised.
   * @param field The field in the entity being de-serialised.
   * @return true if the raw JSON value is acceptable.
   */
  template <typename M>
  bool validate( const char*, const M& ) { return true; }

  /**
   * Rudimentary implementation to check input string for potentially dangerous content.  Dangerous content includes
   * XML/HTML type tags, potential JavaScript code etc.
   *
   * This function is not used when parsing JSON.  This is a stand-alone utility function, which generally is used
   * on the raw JSON data prior to parsing.
   * @param field The string to check for potentially dangerous content.
   * @return true if potentially dangerous content is found.
   */
  bool hasDangerousContent( const std::string& field );
}

template <>
inline bool spt::util::json::validate( const char* name, const std::string_view& field )
{
  return impl::validate( name, field );
}

template <>
inline bool spt::util::json::validate( const char* name, const std::string& field )
{
  return impl::validate( name, std::string_view{ field } );
}

template <>
inline bool spt::util::json::validate( const char* name, const bsoncxx::document::value& field )
{
  return impl::validate( name, field.view() );
}

template <>
inline bool spt::util::json::validate( const char* name, const bsoncxx::document::view& field )
{
  return impl::validate( name, field );
}

template <>
inline bool spt::util::json::validate( const char* name, const bsoncxx::array::value& field )
{
  return impl::validate( name, field.view() );
}

template <>
inline bool spt::util::json::validate( const char* name, const bsoncxx::array::view& field )
{
  return impl::validate( name, field );
}

template <>
inline bool spt::util::json::validate( const char* name, const boost::json::array& field )
{
  return impl::validate( name, field );
}

template <>
inline bool spt::util::json::validate( const char* name, const boost::json::object& field )
{
  return impl::validate( name, field );
}
