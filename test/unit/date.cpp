//
// Created by Rakesh on 11/02/2021.
//

#include "../../src/common/util/date.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace spt::util;
using std::operator""s;
using std::operator""sv;

SCENARIO( "DateTime test suite", "[datetime]" )
{
  GIVEN( "Valid date-time values" )
  {
    WHEN( "Testing round trip conversions" )
    {
      const auto data = GENERATE(
        Catch::Generators::table<std::string_view, std::string_view, int64_t, std::string_view, std::string_view>( {
          { "Time at UNIX epoch"sv, "1970-01-01T00:00:00+00:00"sv, 0, "1970-01-01T00:00:00.000Z"sv, "1970-01-01T00:00:00.000000Z"sv },
          { "Time at 1s past UNIX epoch"sv, "1970-01-01T00:00:01+00:00"sv, 1000000, "1970-01-01T00:00:01.000Z"sv, "1970-01-01T00:00:01.000000Z"sv },
          { "Time at 1m past UNIX epoch"sv, "1970-01-01T00:01:00+00:00"sv, 60000000, "1970-01-01T00:01:00.000Z"sv, "1970-01-01T00:01:00.000000Z"sv },
          { "Time at 1h past UNIX epoch"sv, "1970-01-01T01:00:00+00:00"sv, 3600000000, "1970-01-01T01:00:00.000Z"sv, "1970-01-01T01:00:00.000000Z"sv },
          { "Time at 1d past UNIX epoch"sv, "1970-01-02T00:00:00+00:00"sv, 3600000000 * 24, "1970-01-02T00:00:00.000Z"sv, "1970-01-02T00:00:00.000000Z"sv },
          { "Time at 1 month past UNIX epoch"sv, "1970-02-01T00:00:00+00:00"sv, 3600000000 * 24 * 31, "1970-02-01T00:00:00.000Z"sv, "1970-02-01T00:00:00.000000Z"sv },
          { "Time at 2 months past UNIX epoch"sv, "1970-03-01T00:00:00+00:00"sv, 5097600000 * 1000, "1970-03-01T00:00:00.000Z"sv, "1970-03-01T00:00:00.000000Z"sv },
          { "Time at 3 months past UNIX epoch"sv, "1970-04-01T00:00:00+00:00"sv, 7776000000 * 1000, "1970-04-01T00:00:00.000Z"sv, "1970-04-01T00:00:00.000000Z"sv },
          { "Time at 4 months past UNIX epoch"sv, "1970-05-01T00:00:00+00:00"sv, 10368000000 * 1000, "1970-05-01T00:00:00.000Z"sv, "1970-05-01T00:00:00.000000Z"sv },
          { "Time at 5 months past UNIX epoch"sv, "1970-06-01T00:00:00+00:00"sv, 13046400000 * 1000, "1970-06-01T00:00:00.000Z"sv, "1970-06-01T00:00:00.000000Z"sv },
          { "Time at 6 months past UNIX epoch"sv, "1970-07-01T00:00:00+00:00"sv, 15638400000 * 1000, "1970-07-01T00:00:00.000Z"sv, "1970-07-01T00:00:00.000000Z"sv },
          { "Time at 7 months past UNIX epoch"sv, "1970-08-01T00:00:00+00:00"sv, 18316800000 * 1000, "1970-08-01T00:00:00.000Z"sv, "1970-08-01T00:00:00.000000Z"sv },
          { "Time at 8 months past UNIX epoch"sv, "1970-09-01T00:00:00+00:00"sv, 20995200000 * 1000, "1970-09-01T00:00:00.000Z"sv, "1970-09-01T00:00:00.000000Z"sv },
          { "Time at 9 months past UNIX epoch"sv, "1970-10-01T00:00:00+00:00"sv, 23587200000 * 1000, "1970-10-01T00:00:00.000Z"sv, "1970-10-01T00:00:00.000000Z"sv },
          { "Time at 10 months past UNIX epoch"sv, "1970-11-01T00:00:00+00:00"sv, 26265600000 * 1000, "1970-11-01T00:00:00.000Z"sv, "1970-11-01T00:00:00.000000Z"sv },
          { "Time at 11 months past UNIX epoch"sv, "1970-12-01T00:00:00+00:00"sv, 28857600000 * 1000, "1970-12-01T00:00:00.000Z"sv, "1970-12-01T00:00:00.000000Z"sv },
          { "Time at 1 year past UNIX epoch"sv, "1971-01-01T00:00:00+00:00"sv, 31536000000 * 1000, "1971-01-01T00:00:00.000Z"sv, "1971-01-01T00:00:00.000000Z"sv },
          { "Time at 2 years past UNIX epoch"sv, "1972-01-01T00:00:00+00:00"sv, 63072000000 * 1000, "1972-01-01T00:00:00.000Z"sv, "1972-01-01T00:00:00.000000Z"sv },
          { "Time at 3 years past UNIX epoch"sv, "1973-01-01T00:00:00+00:00"sv, 94694400000 * 1000, "1973-01-01T00:00:00.000Z"sv, "1973-01-01T00:00:00.000000Z"sv },
          { "Time at Y2K"sv, "2000-01-01T00:00:00+00:00"sv, 946684800000 * 1000, "2000-01-01T00:00:00.000Z"sv, "2000-01-01T00:00:00.000000Z"sv }
        } ) );

      const auto [msg, date, expected, millis, micros] = data;
      INFO( msg );
      auto us = std::chrono::microseconds{ microSeconds( date ) };
      CHECK( us.count() == expected );
      CHECK( isoDateMillis( us ) == millis );
      CHECK( isoDateMicros( us ) == micros );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE( var.has_value() );
      CHECK( var->time_since_epoch().count() == expected );
    }

    AND_WHEN( "Testing at some known times" )
    {
      const auto data = GENERATE(
        Catch::Generators::table<std::string_view, std::string_view, int64_t>( {
          { "Time at UTC long"sv, "2015-05-04T02:51:59+00:00"sv, 1430707919000000 },
          { "Time at UTC medium"sv, "2015-05-04T02:51:59+0000"sv, 1430707919000000 },
          { "Time at UTC short"sv, "2015-05-04T02:51:59Z"sv, 1430707919000000 },
          { "Time at one hour past UTC long"sv, "2015-05-04T02:51:59+01:00"sv, 1430704319000000 },
          { "Time at one hour past UTC medium"sv, "2015-05-04T02:51:59+0100"sv, 1430704319000000 },
          { "Time at one hour before UTC long"sv, "2015-05-04T02:51:59-01:00"sv, 1430711519000000 },
          { "Time with millis at UTC long"sv, "2015-05-04T02:51:59.123+00:00"sv, 1430707919123000 },
          { "Time with millis at UTC medium"sv, "2015-05-04T02:51:59.456+0000"sv, 1430707919456000 },
          { "Time with millis at UTC short"sv, "2015-05-04T02:51:59.789Z"sv, 1430707919789000 },
          { "Time with millis at one hour past UTC long"sv, "2015-05-04T02:51:59.123+01:00"sv, 1430704319123000 },
          { "Time with millis at one hour past UTC medium"sv, "2015-05-04T02:51:59.456+0100"sv, 1430704319456000 },
          { "Time with millis at one hour before UTC long"sv, "2015-05-04T02:51:59.789-01:00"sv, 1430711519789000 },
          { "Time with millis at one hour before UTC long"sv, "2015-05-04T02:51:59.238-0100"sv, 1430711519238000 },
          { "Time with micros at UTC long"sv, "2015-05-04T02:51:59.123456+00:00"sv, 1430707919123456 },
          { "Time with micros at UTC medium"sv, "2015-05-04T02:51:59.456789+0000"sv, 1430707919456789 },
          { "Time with micros at UTC short"sv, "2015-05-04T02:51:59.789123Z"sv, 1430707919789123 },
          { "Time with micros at one hour past UTC long"sv, "2015-05-04T02:51:59.123789+01:00"sv, 1430704319123789 },
          { "Time with micros at one hour past UTC medium"sv, "2015-05-04T02:51:59.456231+0100"sv, 1430704319456231 },
          { "Time with micros at one hour before UTC long"sv, "2015-05-04T02:51:59.789654-01:00"sv, 1430711519789654 },
          { "Time with micros at one hour before UTC long"sv, "2015-05-04T02:51:59.238971-0100"sv, 1430711519238971 },
          { "Day only"sv, "2015-05-04"sv, 1430697600000000 },
          { "Date time with simplified millisecond"sv, "2020-10-18T15:01:59.31Z"sv, 1603033319310000 },
          { "Date time with simplified millisecond with zone"sv, "2015-05-04T02:51:59.12+05:30"sv, 1430688119120000 }
        } ) );

      const auto [msg, date, expected] = data;
      INFO( msg );
      const auto var = parseISO8601( date );
      REQUIRE( var.has_value() );
      REQUIRE( var->time_since_epoch().count() ==  expected );
    }

    AND_THEN( "ISO date time converted to UTC" )
    {
      const auto date = "2023-05-18T09:00:00.000-05:00"s;
      const auto var = parseISO8601( date );
      REQUIRE( var.has_value() );

      const auto ts = var->time_since_epoch();
      REQUIRE( isoDateMillis( ts ) == "2023-05-18T14:00:00.000Z" );
    }
  }

  GIVEN( "Invalid date-time values" )
  {
    const auto data = GENERATE(
      Catch::Generators::table<std::string_view, std::string_view>( {
        { "Shorter than minimum day format"sv, "2015-05"sv },
        { "Invalid time part"sv, "2015-05-04T02"sv },
        { "Time with space instead of 'T'"sv, "2015-05-04 02:51:59.238971-0100"sv },
        { "Date time without zone"sv, "2015-05-04T02:51:59"sv },
        { "Date time with invalid zone code"sv, "2015-05-04T02:51:59X"sv },
        { "Date time with invalid zone value"sv, "2015-05-04T02:51:59X"sv },
        { "Date time with invalid zone hour"sv, "2015-05-04T02:51:59+24:30"sv },
        { "Date time with invalid zone minute"sv, "2015-05-04T02:51:59+05:60"sv },
        { "Date time with invalid millisecond"sv, "2015-05-04T02:51:59.1+05:30"sv },
        { "Date time with invalid microsecond"sv, "2015-05-04T02:51:59.1234+05:30"sv }
      } ) );

    const auto [msg, date] = data;
    const auto var = parseISO8601( date );
    REQUIRE_FALSE( var.has_value() );
  }
}
