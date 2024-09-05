//
// Created by Rakesh on 14/12/2023.
//

#include "../../src/common/util/bson.hpp"
#include "../../src/common/util/date.hpp"
#include <catch2/catch_test_macros.hpp>

#include <bsoncxx/builder/list.hpp>

using namespace spt::util;
using std::operator""s;
using std::operator""sv;

SCENARIO( "BSON util functions test suite", "[bson]" )
{
  GIVEN( "BSON document with various types of values" )
  {
    auto builder = bsoncxx::builder::list{
      "string", "string value"s,
      "int32", int32_t{ 123 },
      "int64", int64_t{ 1234567890 },
      "timestamp", int64_t{ 1702571812424000 },
      "date", bsoncxx::types::b_date{ DateTime{ std::chrono::system_clock::now() } }
    };
    auto doc = builder.view().get_document();

    WHEN( "Retrieving values from the document" )
    {
      REQUIRE( bsonValue<std::string>( "string", doc ) == "string value"sv );
      REQUIRE( bsonValue<int32_t>( "int32", doc ) == 123 );
      REQUIRE( bsonValue<int64_t>( "int64", doc ) == 1234567890 );
      REQUIRE( bsonValue<int64_t>( "timestamp", doc ) == 1702571812424000 );
      REQUIRE( bsonValueIfExists<DateTime>( "timestamp", doc ) );
      REQUIRE( bsonValueIfExists<DateTimeMs>( "timestamp", doc ) );
      REQUIRE( bsonValueIfExists<DateTimeNs>( "timestamp", doc ) );
      REQUIRE( bsonValueIfExists<DateTime>( "date", doc ) );
      REQUIRE( bsonValueIfExists<DateTimeMs>( "date", doc ) );
      REQUIRE( bsonValueIfExists<DateTimeMs>( "date", doc ) );
    }
  }

  GIVEN( "A timestamp to generate a BSON oid for" )
  {
    const auto timestamp = std::chrono::system_clock::now() - std::chrono::days{ 30 };
    const auto seed = bsoncxx::oid{};

    WHEN( "Generating object id from the timestamp" )
    {
      auto id = generateId( timestamp, seed );
      auto dest = std::chrono::system_clock::from_time_t( id.get_time_t() );
      auto diff = std::abs( std::chrono::duration_cast<std::chrono::milliseconds>( dest - timestamp ).count() );
      CHECK( diff < 1000 );

      REQUIRE_NOTHROW( bsoncxx::oid{ id.to_string() } );
      REQUIRE_NOTHROW( bsoncxx::oid{ id.bytes(), 12 } );

      CHECK( id == bsoncxx::oid{ id.to_string() } );
      CHECK( id == bsoncxx::oid{ id.bytes(), 12 } );
    }

    AND_WHEN( "Generating object ids with identical timestamp and seed object id returns identical ids" )
    {
      auto orig = generateId( timestamp, seed );

      for ( auto i = 0; i < 100; ++i )
      {
        CHECK( orig == generateId( timestamp, seed ) );
      }
    }

    AND_WHEN( "Generating ids with identical timestamp and different seed ids returns different ids" )
    {
      auto orig = generateId( timestamp, seed );

      for ( auto i = 0; i < 100; ++i )
      {
        CHECK_FALSE( orig == generateId( timestamp ) );
      }
    }
  }
}
