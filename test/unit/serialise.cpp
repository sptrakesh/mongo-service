//
// Created by Rakesh on 12/05/2023.
//

#include "model.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace spt::util;

SCENARIO( "Serialisation test suite", "[serialise]" )
{
  GIVEN( "A fully visitable struct" )
  {
    auto obj = test::serial::Full{};

    WHEN( "Serialising the struct with no data" )
    {
      auto data = marshall( obj );

      THEN( "Serialised BSON does not have empty values" )
      {
        CHECK( data.find( "notVisitable"sv ) != data.cend() );
        auto nv = data["notVisitable"sv].get_document().value;
        REQUIRE( nv.find( "identifier"sv ) != nv.cend() );
        CHECK( bsonValue<std::string>( "identifier"sv, nv ).empty() );
        REQUIRE( nv.find( "integer"sv ) != nv.cend() );
        CHECK( bsonValue<int64_t>( "integer"sv, nv ) == 0 );

        REQUIRE( data.find( "customFields"sv ) != data.cend() );
        auto cf = data["customFields"sv].get_document().value;
        CHECK( cf.find( "id"sv ) == cf.cend() );
        REQUIRE( cf.find( "identifier"sv ) != cf.cend() );
        CHECK( bsonValue<std::string>( "identifier"sv, cf ).empty() );
        CHECK( cf.find( "ref"sv ) == cf.cend() );
        REQUIRE( cf.find( "reference"sv ) != cf.cend() );
        CHECK( bsonValue<bsoncxx::oid>( "reference"sv, cf ) == obj.customFields.ref );

        CHECK( data.find( "identifier"sv ) == data.cend() );
        CHECK( data.find( "nested"sv ) == data.cend() );
        CHECK( data.find( "nesteds"sv ) == data.cend() );
        CHECK( data.find( "nestedp"sv ) == data.cend() );
        CHECK( data.find( "strings"sv ) == data.cend() );
        CHECK( data.find( "ostring"sv ) == data.cend() );
        CHECK( data.find( "obool"sv ) == data.cend() );
        CHECK( data.find( "time"sv ) != data.cend() );
        CHECK( data.find( "_id"sv ) != data.cend() );
        CHECK( data.find( "boolean"sv ) != data.cend() );
      }

      AND_THEN( "Unmarshalled instance does not have data" )
      {
        auto copy = unmarshall<test::serial::Full>( data.view() );
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

      auto data = marshall( obj );

      THEN( "Serialised BSON has all the values" )
      {
        REQUIRE( data.find( "notVisitable"sv ) != data.cend() );
        auto nv = test::serial::NotVisitable{};
        set( nv, bsoncxx::types::bson_value::value{ bsonValue<bsoncxx::document::view>( "notVisitable"sv, data ) } );
        CHECK_THAT( nv.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( nv.integer == obj.notVisitable.integer );

        REQUIRE( data.find( "identifier"sv ) != data.cend() );
        CHECK_THAT( bsonValue<std::string>( "identifier"sv, data.view() ), Catch::Matchers::Equals( obj.identifier ) );
        REQUIRE( data.find( "nested"sv ) != data.cend() );

        auto nested = unmarshall<test::serial::Full::Nested>( bsonValue<bsoncxx::document::view>( "nested"sv, data.view() ) );
        CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nested->identifier ) );
        CHECK( nested.integer == obj.nested->integer );
        CHECK( nested.number == obj.nested->number );
        CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
            std::chrono::duration_cast<std::chrono::milliseconds>( obj.nested->date.time_since_epoch() ) );
        CHECK_FALSE( nested.numbers.empty() );
        CHECK( nested.numbers == obj.nested->numbers );

        auto cs = test::serial::CustomFields{};
        set( cs, bsoncxx::types::bson_value::value{ bsonValue<bsoncxx::document::view>( "customFields"sv, data ) } );
        CHECK_THAT( cs.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( cs.ref == obj.customFields.ref );

        auto nesteds = std::vector<test::serial::Full::Nested>{};
        nesteds.reserve( obj.nesteds.size() );
        for ( const auto& item : bsonValue<bsoncxx::array::view>( "nesteds"sv, data.view() ) )
        {
          nesteds.push_back( unmarshall<test::serial::Full::Nested>( item.get_document().view() ) );
        }

        CHECK( nesteds.size() == obj.nesteds.size() );
        for ( std::size_t i = 0; i < obj.nesteds.size(); ++i )
        {
          CHECK_THAT( nesteds[i].identifier, Catch::Matchers::Equals( obj.nesteds[i].identifier ) );
          CHECK( nesteds[i].integer == obj.nesteds[i].integer );
          CHECK( nesteds[i].number == obj.nesteds[i].number );
          CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nesteds[i].date.time_since_epoch() ) ==
              std::chrono::duration_cast<std::chrono::milliseconds>( obj.nesteds[i].date.time_since_epoch() ) );
          CHECK( nesteds[i].numbers == obj.nesteds[i].numbers );
        }

        REQUIRE( data.find( "nestedp"sv ) != data.cend() );
        nested = unmarshall<test::serial::Full::Nested>( bsonValue<bsoncxx::document::view>( "nestedp"sv, data.view() ) );
        CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nestedp->identifier ) );
        CHECK( nested.integer == obj.nestedp->integer );
        CHECK( nested.number == obj.nestedp->number );
        CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
            std::chrono::duration_cast<std::chrono::milliseconds>( obj.nestedp->date.time_since_epoch() ) );
        CHECK_FALSE( nested.numbers.empty() );
        CHECK( nested.numbers == obj.nestedp->numbers );

        REQUIRE( data.find( "strings"sv ) != data.cend() );
        auto strings = std::vector<std::string>{};
        strings.reserve( obj.strings.size() );
        for ( const auto& str : bsonValue<bsoncxx::array::view>( "strings"sv, data.view() ) ) strings.emplace_back( str.get_string().value );
        CHECK( strings == obj.strings );

        REQUIRE( data.find( "ostring"sv ) != data.cend() );
        CHECK_THAT( bsonValue<std::string>( "ostring"sv, data.view() ), Catch::Matchers::Equals( *obj.ostring ) );
        REQUIRE( data.find( "obool"sv ) != data.cend() );
        CHECK( bsonValue<bool>( "obool"sv, data.view() ) == *obj.obool );
        REQUIRE( data.find( "time"sv ) != data.cend() );
        CHECK(
            std::chrono::duration_cast<std::chrono::milliseconds>( bsonValue<std::chrono::time_point<std::chrono::system_clock>>( "time"sv, data.view() ).time_since_epoch() ) ==
                std::chrono::duration_cast<std::chrono::milliseconds>( obj.time.time_since_epoch() ) );
        REQUIRE( data.find( "boolean"sv ) != data.cend() );
        CHECK( bsonValue<bool>( "boolean"sv, data.view() ) == obj.boolean );
      }

      AND_THEN( "Unmarshalled instance has all the data" )
      {
        auto copy = unmarshall<test::serial::Full>( data.view() );
        CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );
        CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
        CHECK_THAT( copy.customFields.id, Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( copy.customFields.ref == obj.customFields.ref );

        REQUIRE( copy.nested );
        auto& nested = *copy.nested;
        CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nested->identifier ) );
        CHECK( nested.integer == obj.nested->integer );
        CHECK( nested.number == obj.nested->number );
        CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
            std::chrono::duration_cast<std::chrono::milliseconds>( obj.nested->date.time_since_epoch() ) );
        CHECK_FALSE( nested.numbers.empty() );
        CHECK( nested.numbers == obj.nested->numbers );

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
        nested = *copy.nestedp.get();
        CHECK_THAT( nested.identifier, Catch::Matchers::Equals( obj.nestedp->identifier ) );
        CHECK( nested.integer == obj.nestedp->integer );
        CHECK( nested.number == obj.nestedp->number );
        CHECK( std::chrono::duration_cast<std::chrono::milliseconds>( nested.date.time_since_epoch() ) ==
            std::chrono::duration_cast<std::chrono::milliseconds>( obj.nestedp->date.time_since_epoch() ) );
        CHECK_FALSE( nested.numbers.empty() );
        CHECK( nested.numbers == obj.nestedp->numbers );

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
  }

  GIVEN( "A partially visitable struct" )
  {
    static_assert( !visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>(), "Partial is fully visitable" );
    REQUIRE_FALSE( visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>() );
    auto obj = test::serial::Partial{};

    WHEN( "Serialising the structure with no data" )
    {
      auto data = marshall( obj );

      THEN( "Serialised BSON does not have empty properties" )
      {
        CHECK( data.find( "notVisitable"sv ) != data.cend() );

        auto nv = data["notVisitable"sv].get_document().value;
        REQUIRE( nv.find( "identifier"sv ) != nv.cend() );
        CHECK( bsonValue<std::string>( "identifier"sv, nv ).empty() );
        REQUIRE( nv.find( "integer"sv ) != nv.cend() );
        CHECK( bsonValue<int64_t>( "integer"sv, nv ) == 0 );

        REQUIRE( data.find( "customFields"sv ) != data.cend() );
        auto cf = data["customFields"sv].get_document().value;
        CHECK( cf.find( "id"sv ) == cf.cend() );
        REQUIRE( cf.find( "identifier"sv ) != cf.cend() );
        CHECK( bsonValue<std::string>( "identifier"sv, cf ).empty() );
        CHECK( cf.find( "ref"sv ) == cf.cend() );
        REQUIRE( cf.find( "reference"sv ) != cf.cend() );
        CHECK( bsonValue<bsoncxx::oid>( "reference"sv, cf ) == obj.customFields.ref );

        CHECK( data.find( "identifier"sv ) == data.cend() );
        CHECK( data.find( "hidden"sv ) == data.cend() );
        CHECK( data.find( "_id"sv ) != data.cend() );
      }

      AND_THEN( "Unmarshalled instance does not have data" )
      {
        auto copy = unmarshall<test::serial::Partial>( data.view() );
        CHECK( copy.identifier.empty() );
        CHECK( copy.hidden.empty() );
        CHECK( copy.id == obj.id );
      }
    }

    AND_WHEN( "Serialising the structure with data" )
    {
      obj.notVisitable.identifier = "lmn-456";
      obj.notVisitable.integer = -1234;
      obj.customFields.id = "def-2389";
      obj.identifier = "abc123"s;
      obj.hidden = "hidden text";

      auto data = marshall( obj );

      THEN( "Serialised BSON has all the values" )
      {
        REQUIRE( data.find( "notVisitable"sv ) != data.cend() );
        auto nv = test::serial::NotVisitable{};
        set( nv, bsoncxx::types::bson_value::value{ bsonValue<bsoncxx::document::view>( "notVisitable"sv, data ) } );
        CHECK_THAT( nv.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
        CHECK( nv.integer == obj.notVisitable.integer );

        REQUIRE( data.find( "customFields"sv ) != data.cend() );
        auto cf = data["customFields"sv].get_document().value;
        CHECK( cf.find( "id"sv ) == cf.cend() );
        REQUIRE( cf.find( "identifier"sv ) != cf.cend() );
        CHECK_THAT( bsonValue<std::string>( "identifier"sv, cf ), Catch::Matchers::Equals( obj.customFields.id ) );
        CHECK( cf.find( "ref"sv ) == cf.cend() );
        REQUIRE( cf.find( "reference"sv ) != cf.cend() );
        CHECK( bsonValue<bsoncxx::oid>( "reference"sv, cf ) == obj.customFields.ref );

        REQUIRE( data.find( "identifier"sv ) != data.cend() );
        CHECK_THAT( bsonValue<std::string>( "identifier"sv, data.view() ), Catch::Matchers::Equals( obj.identifier ) );
        CHECK( data.find( "hidden"sv ) != data.cend() );
        CHECK_THAT( bsonValue<std::string>( "hidden"sv, data.view() ), Catch::Matchers::Equals( obj.hidden ) );
        REQUIRE( data.find( "_id"sv ) != data.cend() );
        CHECK( bsonValue<bsoncxx::oid>( "_id"sv, data.view() ) == obj.id );
      }

      AND_THEN( "Unmarshalled instance has all the data" )
      {
        auto copy = unmarshall<test::serial::Partial>( data.view() );
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
}
