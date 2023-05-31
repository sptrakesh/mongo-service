//
// Created by Rakesh on 2019-05-29.
//

#pragma once

#include <chrono>
#include <string_view>
#include <variant>

namespace spt::util
{
  using DateTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
  int64_t microSeconds( std::string_view date );
  std::variant<DateTime, std::string> parseISO8601( const std::string_view date );
  std::string isoDateMicros( int64_t epoch );
  std::string isoDateMicros( std::chrono::microseconds epoch );
  std::string isoDateMicros( std::chrono::milliseconds epoch );
  std::string isoDateMicros( const DateTime& epoch );
  std::string isoDateMicros( const std::chrono::time_point<std::chrono::system_clock>& epoch );
  std::string isoDateMillis( int64_t epoch );
  std::string isoDateMillis( std::chrono::microseconds epoch );
  std::string isoDateMillis( std::chrono::milliseconds epoch );
  std::string isoDateMillis( const DateTime& epoch );
}
