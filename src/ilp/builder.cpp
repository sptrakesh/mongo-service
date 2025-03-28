//
// Created by Rakesh on 02/01/2023.
//

#include "builder.hpp"
#include "../common/util/date.hpp"

#include <format>
#include <range/v3/algorithm/for_each.hpp>

using spt::ilp::Builder;

namespace
{
  template<class... Ts> struct overload : Ts... { using Ts::operator()...; };

  namespace pbuilder
  {
    using std::operator ""sv;

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

    std::string_view typeName( spt::ilp::APMRecord::Process::Type type )
    {
      using enum spt::ilp::APMRecord::Process::Type;

      switch ( type )
      {
      case Function: return "function"sv;
      case Step: return "step"sv;
      case Other: return "other"sv;
      default: return ""sv;
      }
    }

    void addAPM( Builder& builder, const spt::ilp::APMRecord& apm )
    {
      auto end = spt::ilp::APMRecord::DateTime{ apm.timestamp + apm.duration };
      builder.
        addTag( "application"sv, apm.application ).
        addValue( "id"sv, apm.id ).
        addValue( "duration"sv, apm.duration.count() ).
        addValue( "end_timestamp", end ).
        timestamp( std::chrono::duration_cast<std::chrono::nanoseconds>( apm.timestamp.time_since_epoch() ) );

      ranges::for_each( apm.tags, [&builder]( const auto& tag ) { builder.addTag( tag.first, tag.second ); } );
      ranges::for_each( apm.values, [&builder]( const auto& pair )
      {
        std::visit( overload
          {
            [&builder, &pair]( bool v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( int64_t v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( uint64_t v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( double v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( const std::string& v ){ builder.addValue( pair.first, v ); },
          }, pair.second );
      } );
    }

    void addProcess( Builder& builder, const spt::ilp::APMRecord::Process& process )
    {
      auto end = spt::ilp::APMRecord::DateTime{ process.timestamp + process.duration };
      builder.
        addTag( "type"sv, typeName( process.type ) ).
        addValue( "duration"sv, process.duration.count() ).
        addValue( "end_timestamp", end ).
        timestamp( std::chrono::duration_cast<std::chrono::nanoseconds>( process.timestamp.time_since_epoch() ) );

      ranges::for_each( process.tags, [&builder]( const auto& tag ) { builder.addTag( tag.first, tag.second ); } );
      ranges::for_each( process.values, [&builder]( const auto& pair )
      {
        std::visit( overload
          {
            [&builder, &pair]( bool v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( int64_t v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( uint64_t v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( double v ){ builder.addValue( pair.first, v ); },
            [&builder, &pair]( const std::string& v ){ builder.addValue( pair.first, v ); },
          }, pair.second );
      } );
    }
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

Builder& Builder::add( std::string_view name, const APMRecord& apm )
{
  using std::operator ""sv;

  startRecord( name );
  pbuilder::addAPM( *this, apm );
  endRecord();
  if ( apm.processes.empty() ) return *this;

  ranges::for_each( apm.processes, [this, name, &apm]( const auto& process )
  {
    startRecord( name ).
      addTag( "application"sv, apm.application ).
      addValue( "id", apm.id );
    pbuilder::addProcess( *this, process );
    endRecord();
  } );
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
