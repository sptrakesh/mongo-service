//
// Created by Rakesh on 11/02/2021.
//

#include "../../src/common/util/date.h"
#include <catch2/catch_test_macros.hpp>

using namespace spt::util;
using std::operator""s;

SCENARIO( "DateTime test suite", "[datetime]" )
{
  GIVEN( "Valid date-time values" )
  {
    WHEN( "Time at UNIX epoch" )
    {
      const auto date = "1970-01-01T00:00:00+00:00"s;
      auto us = microSeconds( date );
      REQUIRE( us == 0 );
      REQUIRE( isoDateMillis( us ) == "1970-01-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-01-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 0 );
    }

    AND_THEN( "Time at 1s past UNIX epoch" )
    {
      const auto date = "1970-01-01T00:00:01+00:00"s;
      const auto expected = 1000000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-01-01T00:00:01.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-01-01T00:00:01.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 1m past UNIX epoch" )
    {
      const auto date = "1970-01-01T00:01:00+00:00"s;
      const auto expected = 60000000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-01-01T00:01:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-01-01T00:01:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 1h past UNIX epoch" )
    {
      const auto date = "1970-01-01T01:00:00+00:00"s;
      const auto expected = 3600000000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-01-01T01:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-01-01T01:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 1d past UNIX epoch" )
    {
      const auto date = "1970-01-02T00:00:00+00:00"s;
      const auto expected = 3600000000 * 24;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-01-02T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-01-02T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 1 month past UNIX epoch" )
    {
      const auto date = "1970-02-01T00:00:00+00:00"s;
      const auto expected = 3600000000 * 24 * 31;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-02-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-02-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 2 months past UNIX epoch" )
    {
      const auto date = "1970-03-01T00:00:00+00:00"s;
      const auto expected = 5097600000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-03-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-03-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 3 months past UNIX epoch" )
    {
      const auto date = "1970-04-01T00:00:00+00:00"s;
      const auto expected = 7776000000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-04-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-04-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 4 months past UNIX epoch" )
    {
      const auto date = "1970-05-01T00:00:00+00:00"s;
      const auto expected = 10368000000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-05-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-05-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 5 months past UNIX epoch" )
    {
      const auto date = "1970-06-01T00:00:00+00:00"s;
      const auto expected = 13046400000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-06-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-06-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 6 months past UNIX epoch" )
    {
      const auto date = "1970-07-01T00:00:00+00:00"s;
      const auto expected = 15638400000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-07-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-07-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 7 months past UNIX epoch" )
    {
      const auto date = "1970-08-01T00:00:00+00:00"s;
      const auto expected = 18316800000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-08-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-08-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 8 months past UNIX epoch" )
    {
      const auto date = "1970-09-01T00:00:00+00:00"s;
      const auto expected = 20995200000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-09-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-09-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 9 months past UNIX epoch" )
    {
      const auto date = "1970-10-01T00:00:00+00:00"s;
      const auto expected = 23587200000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-10-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-10-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 10 months past UNIX epoch" )
    {
      const auto date = "1970-11-01T00:00:00+00:00"s;
      const auto expected = 26265600000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-11-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-11-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 11 months past UNIX epoch" )
    {
      const auto date = "1970-12-01T00:00:00+00:00"s;
      const auto expected = 28857600000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1970-12-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1970-12-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 1 year past UNIX epoch" )
    {
      const auto date = "1971-01-01T00:00:00+00:00"s;
      const auto expected = 31536000000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1971-01-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1971-01-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 2 years past UNIX epoch" )
    {
      const auto date = "1972-01-01T00:00:00+00:00"s;
      const auto expected = 63072000000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1972-01-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1972-01-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at 3 years past UNIX epoch" )
    {
      const auto date = "1973-01-01T00:00:00+00:00"s;
      const auto expected = 94694400000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "1973-01-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "1973-01-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at Y2K" )
    {
      const auto date = "2000-01-01T00:00:00+00:00"s;
      const auto expected = 946684800000 * 1000;
      auto us = microSeconds( date );
      REQUIRE( us == expected );
      REQUIRE( isoDateMillis( us ) == "2000-01-01T00:00:00.000Z"s );
      REQUIRE( isoDateMicros( us ) == "2000-01-01T00:00:00.000000Z"s );

      const auto var = parseISO8601( isoDateMillis( us ) );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == expected );
    }

    AND_THEN( "Time at UTC long" )
    {
      const auto date = "2015-05-04T02:51:59+00:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919000000 );
    }

    AND_THEN( "Time at UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59+0000"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919000000 );
    }

    AND_THEN( "Time at UTC short" )
    {
      const auto date = "2015-05-04T02:51:59Z"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919000000 );
    }

    AND_THEN( "Time at one hour past UTC long" )
    {
      const auto date = "2015-05-04T02:51:59+01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319000000 );
    }

    AND_THEN( "Time at one hour past UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59+0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319000000 );
    }

    AND_THEN( "Time at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59-01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519000000 );
    }

    AND_THEN( "Time at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59-0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519000000 );
    }

    AND_THEN( "Time with millis at UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.123+00:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919123000 );
    }

    AND_THEN( "Time with millis at UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59.456+0000"s;
      const auto var = parseISO8601( date );
      if ( std::holds_alternative<std::string>( var ) )
      {
        UNSCOPED_INFO( std::get<std::string>( var ) );
      }
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919456000 );
    }

    AND_THEN( "Time with millis at UTC short" )
    {
      const auto date = "2015-05-04T02:51:59.789Z"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919789000 );
    }

    AND_THEN( "Time with millis at one hour past UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.123+01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319123000 );
    }

    AND_THEN( "Time with millis at one hour past UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59.456+0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319456000 );
    }

    AND_THEN( "Time with millis at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.789-01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519789000 );
    }

    AND_THEN( "Time with millis at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.238-0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519238000 );
    }

    AND_THEN( "Time with micros at UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.123456+00:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919123456 );
    }

    AND_THEN( "Time with micros at UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59.456789+0000"s;
      const auto var = parseISO8601( date );
      if ( std::holds_alternative<std::string>( var ) )
      {
        UNSCOPED_INFO( std::get<std::string>( var ) );
      }
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919456789 );
    }

    AND_THEN( "Time with micros at UTC short" )
    {
      const auto date = "2015-05-04T02:51:59.789123Z"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430707919789123 );
    }

    AND_THEN( "Time with micros at one hour past UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.123789+01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319123789 );
    }

    AND_THEN( "Time with micros at one hour past UTC medium" )
    {
      const auto date = "2015-05-04T02:51:59.456231+0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430704319456231 );
    }

    AND_THEN( "Time with micros at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.789654-01:00"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519789654 );
    }

    AND_THEN( "Time with micros at one hour before UTC long" )
    {
      const auto date = "2015-05-04T02:51:59.238971-0100"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430711519238971 );
    }

    AND_THEN( "Day only" )
    {
      const auto date = "2015-05-04"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430697600000000 );
    }

    AND_THEN( "Date time with simplified millisecond" )
    {
      const auto date = "2020-10-18T15:01:59.31Z"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1603033319310000 );
    }

    AND_THEN( "Date time with simplified millisecond with zone" )
    {
      const auto date = "2015-05-04T02:51:59.12+05:30"s;
      const auto var = parseISO8601( date );
      REQUIRE_FALSE( std::holds_alternative<std::string>( var ) );
      REQUIRE( std::holds_alternative<DateTime>( var ) );
      REQUIRE( std::get<DateTime>( var ).time_since_epoch().count() == 1430688119120000 );
    }
  }

  GIVEN( "Invalid date-time values" )
  {
    WHEN( "Shorter than minimum day format" )
    {
      const auto date = "2015-05"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Invalid time part" )
    {
      const auto date = "2015-05-04T02"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Time with space instead of 'T'" )
    {
      const auto date = "2015-05-04 02:51:59.238971-0100"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time without zone" )
    {
      const auto date = "2015-05-04T02:51:59"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid zone code" )
    {
      const auto date = "2015-05-04T02:51:59X"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid zone value" )
    {
      const auto date = "2015-05-04T02:51:59X"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid zone hour" )
    {
      const auto date = "2015-05-04T02:51:59+24:30"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid zone minute" )
    {
      const auto date = "2015-05-04T02:51:59+05:60"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid millisecond" )
    {
      const auto date = "2015-05-04T02:51:59.1+05:30"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }

    AND_THEN( "Date time with invalid microsecond" )
    {
      const auto date = "2015-05-04T02:51:59.1234+05:30"s;
      const auto var = parseISO8601( date );
      REQUIRE( std::holds_alternative<std::string>( var ) );
      REQUIRE_FALSE( std::holds_alternative<DateTime>( var ) );
    }
  }
}
