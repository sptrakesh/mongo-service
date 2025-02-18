//
// Created by Rakesh on 02/01/2023.
//

#include "builder.hpp"
#include "../common/util/date.hpp"

#include <format>

using spt::ilp::Builder;

namespace spt::ilp::pbuilder
{
  std::string clean( std::string_view view, bool space = false )
  {
    std::string value{};
    value.reserve( view.size() + 16 );

    for ( std::size_t i = 0; i < view.size(); ++i )
    {
      switch ( view[i] )
      {
      case ',':
      {
        value.append( "\\," );
        break;
      }
      case '"':
      {
        value.append( "\\\"" );
        break;
      }
      case '=':
      {
        value.append( "\\=" );
        break;
      }
      case '\n':
      {
        value.append( "\\\n" );
        break;
      }
      case '\r':
      {
        value.append( "\\\r" );
        break;
      }
      case '\\':
      {
        value.append( 2, '\\' );
        break;
      }
      case ' ':
      {
        if ( space ) value.append( "\\ " );
        else value.append( 1, view[i] );
        break;
      }
      default:
        value.append( 1, view[i] );
      }
    }

    return value;
  }
}

Builder& Builder::startRecord( std::string_view name )
{
  record = Record{ name };
  return *this;
}

Builder& Builder::addTag( std::string_view key, std::string_view v )
{
  if ( !record->tags.empty() ) record->tags.append( "," );
  record->tags.append( std::format( "{}={}", key, pbuilder::clean( v, true ) ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, bool v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, int32_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}i", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, uint32_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}u", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, int64_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}i", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, uint64_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}u", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, float v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, double v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, std::string_view v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}=\"{}\"", key, pbuilder::clean( v, false ) ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, util::DateTime v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}t", key, std::chrono::duration_cast<std::chrono::microseconds>( v.time_since_epoch() ).count() ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, util::DateTimeMs v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}t", key, std::chrono::duration_cast<std::chrono::microseconds>( v.time_since_epoch() ).count() ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, util::DateTimeNs v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( std::format( "{}={}t", key, std::chrono::duration_cast<std::chrono::microseconds>( v.time_since_epoch() ).count() ) );
  return *this;
}

Builder& Builder::timestamp( std::chrono::nanoseconds v )
{
  record->timestamp = v;
  return *this;
}

Builder& Builder::endRecord()
{
  value.reserve( 128 );
  value.append( std::format( "{},{} {} {}\n", record->name, record->tags, record->value, record->timestamp.count() ) );
  return *this;
}

std::string Builder::finish()
{
  return std::move( value );
}
