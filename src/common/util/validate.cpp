//
// Created by Rakesh on 21/02/2024.
//

#include "validate.hpp"

#include <regex>

using std::operator""s;
using std::operator""sv;

bool spt::util::json::hasDangerousContent( const std::string& field )
{
  // "<[A-z0-9_/ \"']*>"s
  // "<(\"[^\"]*\"|'[^']*'|[^'\">])*>"s
  static const auto regex = std::regex{ "<[^<>]+>"s,
      std::regex_constants::ECMAScript | std::regex_constants::icase |
          std::regex::multiline | std::regex::optimize };

  if ( field.empty() ) return false;
  const auto html = [&field]()
  {
    if ( std::regex_search( field, regex ) )
    {
      LOG_WARN << "Potential HTML tag(s) in payload. " << field;
      return true;
    }

    return false;
  };

  const auto script = [&field]()
  {
    const auto var = field.find( "var "sv ) != std::string::npos;
    const auto let = field.find( "let "sv ) != std::string::npos;
    const auto cst = field.find( "const "sv ) != std::string::npos;
    const auto eval = field.find( "eval"sv ) != std::string::npos;
    const auto equals = field.find( "="sv ) != std::string::npos;
    const auto fn = field.find( "function "sv ) != std::string::npos;
    const auto async = field.find( "async "sv ) != std::string::npos;
    const auto await = field.find( "await"sv ) != std::string::npos;
    const auto alert = field.find( "alert("sv ) != std::string::npos;
    const auto console = field.find( "console."sv ) != std::string::npos;
    if ( ( var || let || cst || eval || async || alert ) && ( equals || fn || await || console ) )
    {
      LOG_WARN << "Potential JavaScript in payload. " << field;
      return true;
    }

    return false;
  };

  if ( html() ) return true;
  return script();
}

bool spt::util::json::impl::validate( const char* name, std::string_view field )
{
  if ( field.size() < 2 ) return true;
  using std::operator""sv;
  auto str = std::string{ name };
  boost::algorithm::to_lower( str );
  const auto& helper = IgnoreList::instance();
  for ( const auto& ignore : helper.names ) if ( str.contains( ignore ) ) return true;

  std::size_t special{ 0 };

  for ( const char c : field )
  {
    if ( c < 32 || ( c >= 33 && c <= 47 ) || ( c >= 58 && c <= 64 ) || ( c >= 91 && c <= 96 ) || ( c >= 123 && c < 127 ) ) ++special;
  }

  const auto valid = double(special) / double(field.size()) <= helper.ratio;
  if ( !valid )
  {
    LOG_WARN << "Field " << name << " has too many special characters.  Limit is " << (helper.ratio * 100) << "% of value. Size: " <<
        int(field.size()) << "; special characters: " << int(special) << ". " << field;
  }
  return valid;
}

bool spt::util::json::impl::validate( const char* name, bsoncxx::array::view field )
{
  for ( const auto& e : field )
  {
    switch ( e.type() )
    {
    case bsoncxx::type::k_array:
    {
      auto view = e.get_array().value;
      if ( !validate( name, view ) )
      {
        LOG_WARN << "Invalid data in array of array " << name;
        return false;
      }
      break;
    }
    case bsoncxx::type::k_document:
    {
      auto view = e.get_document().value;
      if ( !validate( name, view ) )
      {
        LOG_WARN << "Invalid data in object of array " << name;
        return false;
      }
      break;
    }
    case bsoncxx::type::k_utf8:
    {
      auto view = e.get_string().value;
      if ( !validate( name, view ) )
      {
        LOG_WARN << "Invalid data in string of array " << name;
        return false;
      }
      break;
    }
    default:
      break;
    }
  }

  return true;
}

bool spt::util::json::impl::validate( const char* name, bsoncxx::document::view field )
{
  for ( const auto& e : field )
  {
    auto k = e.key();
    auto key = std::string{ k };

    switch ( e.type() )
    {
    case bsoncxx::type::k_array:
    {
      auto view = e.get_array().value;
      if ( !validate( key.c_str(), view ) )
      {
        LOG_WARN << "Invalid data in array " << k << " of " << name;
        return false;
      }
      break;
    }
    case bsoncxx::type::k_document:
    {
      auto view = e.get_document().value;
      if ( !validate( key.c_str(), view ) )
      {
        LOG_WARN << "Invalid data in object " << k << " of " << name;
        return false;
      }
      break;
    }
    case bsoncxx::type::k_utf8:
    {
      auto view = e.get_string().value;
      if ( !validate( key.c_str(), view ) )
      {
        LOG_WARN << "Invalid data in string " << k << " of " << name;
        return false;
      }
      break;
    }
    default:
      break;
    }
  }

  return true;
}

bool spt::util::json::impl::validate( const char* name, const boost::json::array& array )
{
  for ( const auto& value : array )
  {
    if ( value.is_array() )
    {
      auto res = validate( name, value.as_array() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in array child of " << name;
        return res;
      }
    }
    else if ( value.is_object() )
    {
      auto res = validate( name, value.as_object() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in object child of " << name;
        return res;
      }
    }
    else if ( value.is_string() )
    {
      auto res = validate( name, value.as_string() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in string child of " << name;
        return res;
      }
    }
  }

  return true;
}

bool spt::util::json::impl::validate( const char* name, const boost::json::object& object )
{
  for ( const auto& [key,value] : object )
  {
    auto kstr = std::string{ key };
    if ( value.is_array() )
    {
      auto res = validate( kstr.c_str(), value.as_array() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in array " << kstr << " child of " << name;
        return res;
      }
    }
    else if ( value.is_object() )
    {
      auto res = validate( kstr.c_str(), value.as_object() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in object " << kstr << " child of " << name;
        return res;
      }
    }
    else if ( value.is_string() )
    {
      auto res = validate( kstr.c_str(), value.as_string() );
      if ( !res )
      {
        LOG_WARN << "Invalid data in string " << kstr << " child of " << name;
        return res;
      }
    }
  }

  return true;
}
