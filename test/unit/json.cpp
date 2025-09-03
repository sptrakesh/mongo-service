//
// Created by Rakesh on 25/05/2023.
//

#include "model.hpp"

#include <sstream>
#include <thread>
#include <boost/uuid/uuid_generators.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace spt::util;

SCENARIO( "JSON Serialisation test suite", "[json]" )
{
  GIVEN( "A fully visitable struct" )
  {
    auto obj = test::serial::Full{};

    WHEN( "JSON serialising the struct with no data" )
    {
      auto data = json::marshall( obj );

      THEN( "Serialised JSON does not have empty values" )
      {
        CHECK( data.contains( "notVisitable"sv ) );
        auto& nv = data["notVisitable"sv].as_object();
        REQUIRE( nv.contains( "identifier"sv ) );
        CHECK( nv["identifier"sv].as_string().empty() );
        REQUIRE( nv.contains( "integer"sv ) );
        CHECK( nv["integer"sv].as_int64() == 0 );

        REQUIRE( data.contains( "customFields"sv ) );
        auto& cf = data["customFields"sv].as_object();
        CHECK_FALSE( cf.contains( "id"sv ) );
        REQUIRE( cf.contains( "identifier"sv ) );
        CHECK( cf["identifier"sv].as_string().empty() );
        CHECK_FALSE( cf.contains( "ref"sv ) );
        REQUIRE( cf.contains( "reference"sv ) );
        CHECK( cf["reference"sv].as_string() == obj.customFields.ref.to_string() );

        CHECK_FALSE( data.contains( "identifier"sv ) );
        CHECK_FALSE( data.contains( "nested"sv ) );
        CHECK_FALSE( data.contains( "nesteds"sv ) );
        CHECK_FALSE( data.contains( "nestedp"sv ) );
        CHECK_FALSE( data.contains( "strings"sv ) );
        CHECK_FALSE( data.contains( "ostring"sv ) );
        CHECK_FALSE( data.contains( "obool"sv ) );
        CHECK( data.contains( "time"sv ) );
        CHECK( data.contains( "id"sv ) );
        CHECK( data.contains( "boolean"sv ) );
      }

      AND_THEN( "Unmarshalled instance does not have data" )
      {
        std::ostringstream ss;
        ss << data;
        auto copy = json::unmarshall<test::serial::Full>( ss.str() );
        CHECK( copy.notVisitable.identifier.empty() );
        CHECK( copy.notVisitable.integer == 0 );
        CHECK( copy.customFields.id.empty() );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK( copy.identifier.empty() );
        CHECK_FALSE( copy.nested );
        CHECK( copy.nesteds.empty() );
        CHECK_FALSE( copy.nestedp );
        CHECK( copy.strings.empty() );
        CHECK_FALSE( copy.ostring );
        CHECK_FALSE( copy.obool );
        CHECK( copy.time == obj.time );
        CHECK( copy.id == obj.id );
        CHECK( copy.boolean == obj.boolean );
      }
    }

    AND_WHEN( "Serialising the struct with data" )
    {
      obj.notVisitable.identifier = "xyz-987";
      obj.notVisitable.integer = 456;
      obj.customFields.id = "lmn-456";
      obj.identifier = "abc-123"s;
      obj.nested = test::serial::Full::Nested{ .identifier = "nested-123"s, .integer = 1234, .number = 1.234, .date = std::chrono::system_clock::now(), .numbers = { 1.2, 2.3, 3.4 }, .level = test::serial::Full::Level::Warning };
      obj.nesteds = {
          test::serial::Full::Nested{ .identifier = "nested-1"s, .integer = 1, .number = 1.1, .date = std::chrono::system_clock::now(), .numbers = { 1.1, 1.2, 1.3 } },
          test::serial::Full::Nested{ .identifier = "nested-2"s, .integer = 2, .number = 2.1, .date = std::chrono::system_clock::now(), .numbers = { 2.1, 2.2, 2.3 } },
          test::serial::Full::Nested{ .identifier = "nested-3"s, .integer = 3, .number = 3.1, .date = std::chrono::system_clock::now(), .numbers = { 3.1, 3.2, 3.3 } }
      };
      obj.nestedp = std::make_shared<test::serial::Full::Nested>();
      obj.nestedp->identifier = "nested-p"s;
      obj.nestedp->integer = 234;
      obj.nestedp->number = 234.567;
      obj.nestedp->date = std::chrono::system_clock::now();
      obj.nestedp->numbers = { 1.2, 2.3, 3.4 };
      obj.strings = { "one"s, "two"s, "three"s };
      obj.ostring = "some string value"s;
      obj.obool = true;
      obj.time = std::chrono::system_clock::now();
      obj.boolean = true;

      auto data = json::marshall( obj );

      THEN( "Serialised JSON has all the values" )
      {
        REQUIRE( data.contains( "notVisitable"sv ) );
        REQUIRE( data["notVisitable"sv].is_object() );
        auto& nv = data["notVisitable"sv].as_object();
        REQUIRE( nv.contains( "identifier"sv ) );
        CHECK( nv["identifier"sv].as_string() == obj.notVisitable.identifier );
        CHECK( nv["integer"sv].as_int64() == obj.notVisitable.integer );

        REQUIRE( data.contains( "identifier"sv ) );
        CHECK( data["identifier"sv].as_string() == obj.identifier );

        REQUIRE( data.contains( "nested"sv ) );
        REQUIRE( data["nested"sv].is_object() );
        {
          auto& nested = data["nested"sv].as_object();
          CHECK( nested["identifier"sv].as_string() == obj.nested->identifier );
          CHECK( nested["integer"sv] == obj.nested->integer );
          CHECK( nested["number"sv] == obj.nested->number );
          auto date = isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( obj.nested->date.time_since_epoch() ) );
          CHECK( nested["date"sv].as_string() == date );
          REQUIRE( nested.contains( "numbers"sv ) );
          REQUIRE( nested["numbers"sv].is_array() );
          auto& arr = nested["numbers"sv].as_array();
          REQUIRE( arr.size() == obj.nested->numbers.size() );
          for ( std::size_t i = 0; i < arr.size(); ++i ) CHECK( arr[i].as_double() == obj.nested->numbers[i] );
        }

        REQUIRE( data.contains( "customFields"sv ) );
        REQUIRE( data["customFields"sv].is_object() );
        auto& cs = data["customFields"sv].as_object();
        CHECK( cs["identifier"sv].as_string() == obj.customFields.id );
        CHECK( cs["reference"sv].as_string() == obj.customFields.ref.to_string() );

        REQUIRE( data.contains( "nesteds"sv ) );
        REQUIRE( data["nesteds"sv].is_array() );
        {
          auto& arr = data["nesteds"sv].as_array();
          REQUIRE( arr.size() == obj.nesteds.size() );
          for ( std::size_t i = 0; i < arr.size(); ++i )
          {
            REQUIRE( arr[i].is_object() );
            auto& n = arr[i].as_object();
            CHECK( n["identifier"sv].as_string() == obj.nesteds[i].identifier );
            CHECK( n["integer"sv].as_int64() == obj.nesteds[i].integer );
            CHECK( n["number"sv].as_double() == obj.nesteds[i].number );
            auto dt = isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( obj.nesteds[i].date.time_since_epoch() ) );
            CHECK( n["date"sv].as_string() == dt );
            CHECK( n["numbers"sv].as_array().size() == obj.nesteds[i].numbers.size() );
          }
        }

        REQUIRE( data.contains( "nestedp"sv ) );
        REQUIRE( data["nestedp"sv].is_object() );
        {
          auto& nested = data["nestedp"sv].as_object();
          CHECK( nested["identifier"sv].as_string() == obj.nestedp->identifier );
          CHECK( nested["integer"sv] == obj.nestedp->integer );
          CHECK( nested["number"sv] == obj.nestedp->number );
          auto date = isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( obj.nestedp->date.time_since_epoch() ) );
          CHECK( nested["date"sv].as_string() == date );
          REQUIRE( nested.contains( "numbers"sv ) );
          REQUIRE( nested["numbers"sv].is_array() );
          auto& arr = nested["numbers"sv].as_array();
          REQUIRE( arr.size() == obj.nestedp->numbers.size() );
          for ( std::size_t i = 0; i < arr.size(); ++i ) CHECK( arr[i].as_double() == obj.nestedp->numbers[i] );
        }

        REQUIRE( data.find( "strings"sv ) != data.cend() );
        auto strings = std::vector<std::string>{};
        strings.reserve( obj.strings.size() );
        for ( const auto& item : data["strings"sv].as_array() ) strings.emplace_back( item.as_string() );
        CHECK( strings == obj.strings );

        REQUIRE( data.contains( "ostring"sv ) );
        CHECK( data["ostring"sv].as_string() == *obj.ostring );
        REQUIRE( data.contains( "obool"sv ) );
        CHECK( data["obool"sv].as_bool() == *obj.obool );
        REQUIRE( data.contains( "time"sv ) );
        CHECK( data["time"sv].as_string() == isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( obj.time.time_since_epoch() ) ) );
        REQUIRE( data.contains( "boolean"sv ) );
        CHECK( data["boolean"sv].as_bool() == obj.boolean );
      }

      AND_THEN( "Unmarshalled instance has all the data" )
      {
        auto ss = std::ostringstream{};
        ss << data;
        auto copy = json::unmarshall<test::serial::Full>( ss.str() );
        CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );

        REQUIRE( copy.nested );
        {
          auto& nested = *copy.nested;
          CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nested->identifier ) );
          CHECK( nested.integer == obj.nested->integer );
          CHECK( nested.number == obj.nested->number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nested->date.time_since_epoch() ) );
          CHECK_FALSE( nested.numbers.empty() );
          CHECK( nested.numbers == obj.nested->numbers );
        }

        CHECK( copy.nesteds.size() == obj.nesteds.size() );
        for ( std::size_t i = 0; i < obj.nesteds.size(); ++i )
        {
          CHECK_THAT( copy.nesteds[i].identifier, Catch::Matchers::Equals( obj.nesteds[i].identifier ) );
          CHECK( copy.nesteds[i].integer == obj.nesteds[i].integer );
          CHECK( copy.nesteds[i].number == obj.nesteds[i].number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( copy.nesteds[i].date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nesteds[i].date.time_since_epoch() ) );
          CHECK( copy.nesteds[i].numbers == obj.nesteds[i].numbers );
        }

        REQUIRE( copy.nestedp );
        {
          auto& nested = *copy.nestedp;
          CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nestedp->identifier ) );
          CHECK( nested.integer == obj.nestedp->integer );
          CHECK( nested.number == obj.nestedp->number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nestedp->date.time_since_epoch() ) );
          CHECK_FALSE( nested.numbers.empty() );
          CHECK( nested.numbers == obj.nestedp->numbers );
        }

        CHECK( copy.strings == obj.strings );
        CHECK( copy.ostring );
        CHECK_THAT( *copy.ostring, Catch::Matchers::Equals( *obj.ostring ) );
        CHECK( copy.obool );
        CHECK( *copy.obool == *obj.obool );
        CHECK( copy.id == obj.id );
        CHECK(
            std::chrono::duration_cast<std::chrono::milliseconds>( copy.time.time_since_epoch() ) ==
                std::chrono::duration_cast<std::chrono::milliseconds>( obj.time.time_since_epoch() ) );
        CHECK( copy.boolean == obj.boolean );
      }
    }

    AND_WHEN( "Converting the struct to string" )
    {
      auto data = json::str( obj );

      THEN( "Unmarshalling the struct from string" )
      {
        auto copy = json::unmarshall<test::serial::Full>( data );
        CHECK( copy.notVisitable.identifier.empty() );
        CHECK( copy.notVisitable.integer == 0 );
        CHECK( copy.customFields.id.empty() );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK( copy.identifier.empty() );
        CHECK_FALSE( copy.nested );
        CHECK( copy.nesteds.empty() );
        CHECK_FALSE( copy.nestedp );
        CHECK( copy.strings.empty() );
        CHECK_FALSE( copy.ostring );
        CHECK_FALSE( copy.obool );
        CHECK( copy.time == obj.time );
        CHECK( copy.id == obj.id );
        CHECK( copy.boolean == obj.boolean );
      }

      AND_THEN( "Streaming the object into a string" )
      {
        using spt::util::json::operator<<;
        std::ostringstream ss;
        ss << obj;
        CHECK_FALSE( ss.str().empty() );
      }
    }

    AND_WHEN( "Converting the struct with data to string" )
    {
      obj.notVisitable.identifier = "xyz-987";
      obj.notVisitable.integer = 456;
      obj.customFields.id = "lmn-456";
      obj.identifier = "abc-123"s;
      obj.nested = test::serial::Full::Nested{ .identifier = "nested-123"s, .integer = 1234, .number = 1.234, .date = std::chrono::system_clock::now(), .numbers = { 1.2, 2.3, 3.4 } };
      obj.nesteds = {
          test::serial::Full::Nested{ .identifier = "nested-1"s, .integer = 1, .number = 1.1, .date = std::chrono::system_clock::now(), .numbers = { 1.1, 1.2, 1.3 } },
          test::serial::Full::Nested{ .identifier = "nested-2"s, .integer = 2, .number = 2.1, .date = std::chrono::system_clock::now(), .numbers = { 2.1, 2.2, 2.3 } },
          test::serial::Full::Nested{ .identifier = "nested-3"s, .integer = 3, .number = 3.1, .date = std::chrono::system_clock::now(), .numbers = { 3.1, 3.2, 3.3 } }
      };
      obj.nestedp = std::make_shared<test::serial::Full::Nested>();
      obj.nestedp->identifier = "nested-p"s;
      obj.nestedp->integer = 234;
      obj.nestedp->number = 234.567;
      obj.nestedp->date = std::chrono::system_clock::now();
      obj.nestedp->numbers = { 1.2, 2.3, 3.4 };
      obj.strings = { "one"s, "two"s, "three"s };
      obj.ostring = "some string value"s;
      obj.obool = true;
      obj.time = std::chrono::system_clock::now();
      obj.boolean = true;

      auto data = json::str( obj );

      THEN( "Unmarshalling the struct from string" )
      {
        auto copy = json::unmarshall<test::serial::Full>( data );
        CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );

        REQUIRE( copy.nested );
        {
          auto& nested = *copy.nested;
          CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nested->identifier ) );
          CHECK( nested.integer == obj.nested->integer );
          CHECK( nested.number == obj.nested->number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nested->date.time_since_epoch() ) );
          CHECK_FALSE( nested.numbers.empty() );
          CHECK( nested.numbers == obj.nested->numbers );
        };

        CHECK( copy.nesteds.size() == obj.nesteds.size() );
        for ( std::size_t i = 0; i < obj.nesteds.size(); ++i )
        {
          CHECK_THAT( copy.nesteds[i].identifier, Catch::Matchers::Equals( obj.nesteds[i].identifier ) );
          CHECK( copy.nesteds[i].integer == obj.nesteds[i].integer );
          CHECK( copy.nesteds[i].number == obj.nesteds[i].number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( copy.nesteds[i].date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nesteds[i].date.time_since_epoch() ) );
          CHECK( copy.nesteds[i].numbers == obj.nesteds[i].numbers );
        }

        REQUIRE( copy.nestedp );
        {
          auto& nested = *copy.nestedp.get();
          CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nestedp->identifier ) );
          CHECK( nested.integer == obj.nestedp->integer );
          CHECK( nested.number == obj.nestedp->number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nestedp->date.time_since_epoch() ) );
          CHECK_FALSE( nested.numbers.empty() );
          CHECK( nested.numbers == obj.nestedp->numbers );
        };

        CHECK( copy.strings == obj.strings );
        CHECK( copy.ostring );
        CHECK_THAT( *copy.ostring, Catch::Matchers::Equals( *obj.ostring ) );
        CHECK( copy.obool );
        CHECK( *copy.obool == *obj.obool );
        CHECK( copy.id == obj.id );
        CHECK(
            std::chrono::duration_cast<std::chrono::milliseconds>( copy.time.time_since_epoch() ) ==
                std::chrono::duration_cast<std::chrono::milliseconds>( obj.time.time_since_epoch() ) );
        CHECK( copy.boolean == obj.boolean );
      }

      AND_THEN( "Streaming the object into a string" )
      {
        using spt::util::json::operator<<;
        std::ostringstream ss;
        ss << obj;
        CHECK_FALSE( ss.str().empty() );
      }
    }

    AND_WHEN( "Parsing data with invalid characters" )
    {
      using spt::util::json::operator<<;
      obj.strings = { "one"s, "two"s, "three"s, "$%^$%)*&%*&(&&*(**&"s };

      std::ostringstream ss;
      ss << obj;

      REQUIRE_THROWS( json::unmarshall<test::serial::Full>( ss.str() ) );
    }

    AND_WHEN( "Parsing data with partial invalid characters" )
    {
      using spt::util::json::operator<<;
      obj.strings = { "one"s, "two"s, "three"s, "$%^$%abcde0gzyx234"s };

      std::ostringstream ss;
      ss << obj;

      REQUIRE_NOTHROW( json::unmarshall<test::serial::Full>( ss.str() ) );
    }
  }

  GIVEN( "A vector of fully visitable structs" )
  {
    auto arr = boost::json::array{};
    arr.reserve( 4 );

    for ( auto i = 0ul; i < 4; ++i )
    {
      auto obj = test::serial::Full{};
      obj.notVisitable.identifier = "xyz-987";
      obj.notVisitable.integer = 456;
      obj.customFields.id = "lmn-456";
      obj.identifier = "abc-123"s;
      obj.nested = test::serial::Full::Nested{ .identifier = "nested-123"s, .integer = 1234, .number = 1.234, .date = std::chrono::system_clock::now(), .numbers = { 1.2, 2.3, 3.4 } };
      obj.nesteds = {
          test::serial::Full::Nested{ .identifier = "nested-1"s, .integer = 1, .number = 1.1, .date = std::chrono::system_clock::now(), .numbers = { 1.1, 1.2, 1.3 } },
          test::serial::Full::Nested{ .identifier = "nested-2"s, .integer = 2, .number = 2.1, .date = std::chrono::system_clock::now(), .numbers = { 2.1, 2.2, 2.3 } },
          test::serial::Full::Nested{ .identifier = "nested-3"s, .integer = 3, .number = 3.1, .date = std::chrono::system_clock::now(), .numbers = { 3.1, 3.2, 3.3 } }
      };
      obj.nestedp = std::make_shared<test::serial::Full::Nested>();
      obj.nestedp->identifier = "nested-p"s;
      obj.nestedp->integer = 234;
      obj.nestedp->number = 234.567;
      obj.nestedp->date = std::chrono::system_clock::now();
      obj.nestedp->numbers = { 1.2, 2.3, 3.4 };
      obj.strings = { "one"s, "two"s, "three"s };
      obj.ostring = "some string value"s;
      obj.obool = true;
      obj.time = std::chrono::system_clock::now();
      obj.boolean = true;
      arr.emplace_back( json::marshall( obj ) );
    }

    THEN( "Unmarshalling array to vector" )
    {
      auto vec = std::vector<test::serial::Full>{};
      json::unmarshall( vec, boost::json::serialize( arr ) );
      CHECK( vec.size() == 4 );
      for ( const auto& obj : vec )
      {
        CHECK( obj.notVisitable.identifier == "xyz-987" );
        CHECK( obj.notVisitable.integer == 456 );
        CHECK( obj.customFields.id == "lmn-456" );
        CHECK( obj.identifier == "abc-123"s );
        REQUIRE( obj.nested );
        CHECK( obj.nested->identifier == "nested-123" );
        CHECK( obj.nested->integer == 1234 );
        CHECK( obj.nested->number == 1.234 );
        REQUIRE( obj.nested->numbers.size() == 3 );
        CHECK( obj.nested->numbers[0] == 1.2 );
        CHECK( obj.nested->numbers[1] == 2.3 );
        CHECK( obj.nested->numbers[2] == 3.4 );
        REQUIRE( obj.nestedp );
        CHECK( obj.nestedp->identifier == "nested-p" );
        CHECK( obj.nestedp->integer == 234 );
        CHECK( obj.nestedp->number == 234.567 );
        REQUIRE( obj.nestedp->numbers.size() == 3 );
        CHECK( obj.nestedp->numbers[0] == 1.2 );
        CHECK( obj.nestedp->numbers[1] == 2.3 );
        CHECK( obj.nestedp->numbers[2] == 3.4 );
        REQUIRE( obj.strings.size() == 3 );
        CHECK( obj.strings[0] == "one" );
        CHECK( obj.strings[1] == "two" );
        CHECK( obj.strings[2] == "three" );
        REQUIRE( obj.ostring );
        CHECK( *obj.ostring == "some string value" );
        REQUIRE( obj.obool );
        CHECK( *obj.obool );
      }
    }
  }

  GIVEN( "A partially visitable struct" )
  {
    static_assert( !visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>(), "Partial is fully visitable" );
    REQUIRE_FALSE( visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>() );
    auto obj = test::serial::Partial{};

    WHEN( "JSON Serialising the structure with no data" )
    {
      auto data = json::marshall( obj );

      THEN( "Serialised JSON does not have empty properties" )
      {
        REQUIRE( data.contains( "notVisitable"sv ) );

        auto& nv = data["notVisitable"sv].as_object();
        REQUIRE( nv.contains( "identifier"sv ) );
        CHECK( nv["identifier"sv].as_string().empty() );
        REQUIRE( nv.contains( "integer"sv ) );
        CHECK( nv["integer"sv].as_int64() == 0 );

        REQUIRE( data.contains( "customFields"sv ) );
        auto& cf = data["customFields"sv].as_object();
        REQUIRE( cf.contains( "identifier"sv ) );
        CHECK( cf["identifier"sv].as_string().empty() );
        CHECK_FALSE( cf.contains( "ref"sv ) );
        REQUIRE( cf.contains( "reference"sv ) );
        CHECK( cf["reference"sv].as_string() == obj.customFields.ref.to_string() );

        CHECK_FALSE( data.contains( "identifier"sv ) );
        CHECK_FALSE( data.contains( "hidden"sv ) );
        REQUIRE( data.contains( "id"sv ) );
        CHECK( data["id"sv].as_string() == obj.id.to_string() );
      }

      AND_THEN( "Unmarshalled instance does not have data" )
      {
        auto ss = std::ostringstream{};
        ss << data;
        auto copy = json::unmarshall<test::serial::Partial>( ss.str() );
        CHECK( copy.identifier.empty() );
        CHECK( copy.hidden.empty() );
        CHECK( copy.id == obj.id );
      }
    }

    AND_WHEN( "JSON Serialising the structure with data" )
    {
      obj.notVisitable.identifier = "lmn-456";
      obj.notVisitable.integer = -1234;
      obj.customFields.id = "def-2389";
      obj.identifier = "abc123"s;
      obj.hidden = "hidden text";

      auto data = json::marshall( obj );

      THEN( "Serialised BSON has all the values" )
      {
        REQUIRE( data.contains( "notVisitable"sv ) );
        auto& nv = data["notVisitable"sv].as_object();
        CHECK( nv["identifier"sv].as_string() == obj.notVisitable.identifier );
        CHECK( nv["integer"sv].as_int64() == obj.notVisitable.integer );

        REQUIRE( data.contains( "customFields"sv ) );
        auto& cf = data["customFields"sv].as_object();
        REQUIRE( cf.contains( "identifier"sv ) );
        CHECK( cf["identifier"sv].as_string() == obj.customFields.id );
        CHECK_FALSE( cf.contains( "ref"sv ) );
        REQUIRE( cf.contains( "reference"sv ) );
        CHECK( cf["reference"sv].as_string() == obj.customFields.ref.to_string() );

        REQUIRE( data.contains( "identifier"sv ) );
        CHECK( data["identifier"sv].as_string() == obj.identifier );
        REQUIRE( data.contains( "hidden"sv ) );
        CHECK( data["hidden"sv].as_string() == obj.hidden );
        REQUIRE( data.contains( "id"sv ) );
        CHECK( data["id"sv].as_string() == obj.id.to_string() );
      }

      AND_THEN( "Unmarshalled instance has all the data" )
      {
        auto ss = std::ostringstream{};
        ss << data;
        auto copy = json::unmarshall<test::serial::Partial>( ss.str() );
        CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
        CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK_THAT( copy.hidden, Catch::Matchers::Equals( obj.hidden ) );
        CHECK( copy.id == obj.id );
      }
    }
  }

  GIVEN( "A UUID value" )
  {
    auto generator = boost::uuids::random_generator{};
    auto uuid = generator();
    auto value = json::json( uuid );
    CHECK( value.as_string() == boost::lexical_cast<std::string>( uuid  ) );
  }
}

SCENARIO( "Pooled parser test suite", "[pooled-parser]" )
{
  GIVEN( "A fully visitable struct" )
  {
    const auto obj = test::serial::Full{};
    const auto data = json::str( obj );

    WHEN( "Deserialising data in a loop" )
    {
      for ( auto i = 0; i < 100; ++i ) json::unmarshall<test::serial::Full>( data );
    }

    AND_WHEN( "Deserialing data in parallel" )
    {
      auto threads = std::vector<std::thread>{};
      threads.reserve( 100 );
      for ( auto i = 0; i < 100; ++i ) threads.emplace_back( [&data](){ json::unmarshall<test::serial::Full>( data ); } );
      for ( auto i = 0; i < 100; ++i ) if ( threads[i].joinable() ) threads[i].join();
    }
  }
}

SCENARIO( "Properties test suite", "[properties]" )
{
  GIVEN( "A JSON representation" )
  {
    const auto json = R"json({
  "properties": [
    {
      "name": "begin",
      "value": {
        "unit": "in",
        "value": 4.114543762687808E3
      }
    },
    {
      "name": "end",
      "value": {
        "unit": "in",
        "value": 2.8749617758545774E3
      }
    }
  ]
})json";

    WHEN( "Parsing JSON" )
    {
      const auto p = json::unmarshall<test::serial::Properties>( json );
      REQUIRE( p.properties.size() == 2 );
      CHECK( p.properties[0].name == "begin" );
      REQUIRE( p.properties[0].value.is_object() );

      const auto& p0 = p.properties[0].value.as_object();
      REQUIRE( p0.contains( "unit" ) );
      CHECK( p0.at( "unit" ).as_string() == "in" );
      REQUIRE( p0.contains( "value" ) );
      CHECK_THAT( p0.at( "value" ).as_double(), Catch::Matchers::WithinRel( 4.114543762687808E3, 0.000001 ) );

      CHECK( p.properties[1].name == "end" );
      REQUIRE( p.properties[1].value.is_object() );
      const auto& p1 = p.properties[1].value.as_object();
      REQUIRE( p1.contains( "unit" ) );
      CHECK( p1.at( "unit" ).as_string() == "in" );
      REQUIRE( p1.contains( "value" ) );
      CHECK_THAT( p1.at( "value" ).as_double(), Catch::Matchers::WithinRel( 2.8749617758545774E3, 0.000001 ) );
    }

    AND_WHEN( "Serialising to JSON" )
    {
      const auto _p = json::unmarshall<test::serial::Properties>( json );
      const auto str = json::str( _p );
      const auto p = json::unmarshall<test::serial::Properties>( str );

      const auto& p0 = p.properties[0].value.as_object();
      REQUIRE( p0.contains( "unit" ) );
      CHECK( p0.at( "unit" ).as_string() == "in" );
      REQUIRE( p0.contains( "value" ) );
      CHECK_THAT( p0.at( "value" ).as_double(), Catch::Matchers::WithinRel( 4.114543762687808E3, 0.000001 ) );

      CHECK( p.properties[1].name == "end" );
      REQUIRE( p.properties[1].value.is_object() );
      const auto& p1 = p.properties[1].value.as_object();
      REQUIRE( p1.contains( "unit" ) );
      CHECK( p1.at( "unit" ).as_string() == "in" );
      REQUIRE( p1.contains( "value" ) );
      CHECK_THAT( p1.at( "value" ).as_double(), Catch::Matchers::WithinRel( 2.8749617758545774E3, 0.000001 ) );
    }

    AND_WHEN( "Serialising to BSON" )
    {
      const auto _p = json::unmarshall<test::serial::Properties>( json );
      const auto bson = marshall( _p );
      const auto p = unmarshall<test::serial::Properties>( bson );

      const auto& p0 = p.properties[0].value.as_object();
      REQUIRE( p0.contains( "unit" ) );
      CHECK( p0.at( "unit" ).as_string() == "in" );
      REQUIRE( p0.contains( "value" ) );
      CHECK_THAT( p0.at( "value" ).as_double(), Catch::Matchers::WithinRel( 4.114543762687808E3, 0.000001 ) );

      CHECK( p.properties[1].name == "end" );
      REQUIRE( p.properties[1].value.is_object() );
      const auto& p1 = p.properties[1].value.as_object();
      REQUIRE( p1.contains( "unit" ) );
      CHECK( p1.at( "unit" ).as_string() == "in" );
      REQUIRE( p1.contains( "value" ) );
      CHECK_THAT( p1.at( "value" ).as_double(), Catch::Matchers::WithinRel( 2.8749617758545774E3, 0.000001 ) );
    }
  }
}