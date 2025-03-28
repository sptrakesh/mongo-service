//
// Created by Rakesh on 17/03/2025.
//

#include "apmrecord.hpp"

using spt::ilp::APMRecord;

APMRecord spt::ilp::createAPMRecord( std::string_view id, std::string_view application,
    APMRecord::Process::Type type, std::size_t size, const std::source_location loc )
{
  auto apm = APMRecord{ id };
  apm.application = application;
  apm.processes.reserve( size );
  apm.processes.emplace_back( type );
  {
    auto& p = apm.processes.back();
    p.values.try_emplace( "file", loc.file_name() );
    p.values.try_emplace( "line", static_cast<uint64_t>( loc.line() ) );
    if ( auto fn = std::string{ loc.function_name() }; !fn.empty() ) p.values.try_emplace( "function", std::move( fn ) );
  }

  return apm;
}

APMRecord::Process& spt::ilp::addProcess( APMRecord& apm, APMRecord::Process::Type type, const std::source_location loc )
{
  apm.processes.emplace_back( type );
  auto& p = apm.processes.back();
  p.values.try_emplace( "file", loc.file_name() );
  p.values.try_emplace( "line", static_cast<uint64_t>( loc.line() ) );
  if ( auto fn = std::string{ loc.function_name() }; !fn.empty() ) p.values.try_emplace( "function", std::move( fn ) );
  return p;
}

APMRecord::Process& spt::ilp::addException( APMRecord& apm, const std::exception& ex, std::string_view prefix, const std::source_location loc )
{
  apm.processes.emplace_back( APMRecord::Process::Type::Step );
  auto& p = apm.processes.back();
  p.values.try_emplace( "file", loc.file_name() );
  p.values.try_emplace( "line", static_cast<uint64_t>( loc.line() ) );
  if ( auto fn = std::string{ loc.function_name() }; !fn.empty() )
  {
    p.values.try_emplace( "function", fn );
    p.tags.try_emplace( "parent", std::move( fn ) );
  }
  p.values.try_emplace( "error", std::format( "{}. {}", prefix, ex.what() ) );
  return p;
}
