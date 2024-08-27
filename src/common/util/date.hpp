//
// Created by Rakesh on 2019-05-29.
//

#pragma once

#include <chrono>
#include <expected>
#include <string_view>

namespace spt::util
{
  using DateTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
  using DateTimeMs = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
  using DateTimeNs = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;

  int64_t microSeconds( std::string_view date );
  std::expected<DateTime, std::string> parseISO8601( std::string_view date );
  [[deprecated("Use the variants that take std::chrono")]] std::string isoDateMicros( int64_t epoch );
  std::string isoDateMicros( std::chrono::microseconds epoch );
  std::string isoDateMicros( std::chrono::milliseconds epoch );
  std::string isoDateMicros( const DateTime& epoch );
  std::string isoDateMicros( const DateTimeMs& epoch );
  std::string isoDateMicros( const DateTimeNs& epoch );
  [[deprecated("Use the variants that take std::chrono")]] std::string isoDateMillis( int64_t epoch );
  std::string isoDateMillis( std::chrono::microseconds epoch );
  std::string isoDateMillis( std::chrono::milliseconds epoch );
  std::string isoDateMillis( const DateTime& epoch );
  std::string isoDateMillis( const DateTimeMs& epoch );
  std::string isoDateMillis( const DateTimeNs& epoch );
}
