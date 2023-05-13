//
// Created by Rakesh on 12/05/2023.
//

#include "../../src/common/util/bson.h"
#include "../../src/common/util/serialise.h"
#include "../../src/common/visit_struct/fully_visitable.hpp"
#include "../../src/common/visit_struct/visit_struct_intrusive.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using std::operator""s;
using std::operator""sv;

using namespace spt::util;

namespace spt::util::test::serial
{
  struct NotVisitable
  {
    NotVisitable() = default;
    ~NotVisitable() = default;
    explicit NotVisitable( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, identifier, bson )
      FROM_BSON( int64_t, integer, bson )
    }

    std::string identifier;
    int64_t integer;
  };

  bsoncxx::types::bson_value::value bson( const NotVisitable& model )
  {
    return {
      bsoncxx::builder::stream::document{} <<
        "identifier" << model.identifier <<
        "integer" << model.integer <<
        bsoncxx::builder::stream::finalize
    };
  }

  void set( NotVisitable& field, bsoncxx::types::bson_value::view value )
  {
    field = NotVisitable{ value.get_document().value };
  }

  struct Full
  {
    struct Nested
    {
      BEGIN_VISITABLES(Nested);
      VISITABLE(std::string, identifier);
      VISITABLE(int32_t, integer);
      VISITABLE(double, number);
      VISITABLE(std::chrono::time_point<std::chrono::system_clock>, date);
      VISITABLE(std::vector<double>, numbers);
      END_VISITABLES;
    };

    BEGIN_VISITABLES(Full);
    VISITABLE(NotVisitable, notVisitable);
    VISITABLE(std::string, identifier);
    VISITABLE(std::optional<Nested>, nested);
    VISITABLE(std::vector<Nested>, nesteds);
    VISITABLE(std::vector<std::string>, strings);
    VISITABLE(std::optional<std::string>, ostring);
    VISITABLE(std::optional<bool>, obool);
    VISITABLE(bsoncxx::oid, id);
    VISITABLE(bool, boolean);
    END_VISITABLES;
  };

  struct Partial
  {
    BEGIN_VISITABLES(Partial);
    VISITABLE(NotVisitable, notVisitable);
    VISITABLE(std::string, identifier);
    std::string hidden;
    VISITABLE(bsoncxx::oid, id);
    END_VISITABLES;
  };
}

SCENARIO( "Serialisation test suite", "[serialise]" )
{
  GIVEN( "A fully visitable struct" )
  {
    auto obj = test::serial::Full{};

    WHEN( "Serialising the struct with no data" )
    {
      auto data = marshall( obj );

      CHECK( data.find( "notVisitable"sv ) != data.cend() );
      auto nv = data["notVisitable"sv].get_document().value;
      REQUIRE( nv.find( "identifier"sv ) != nv.cend() );
      CHECK( bsonValue<std::string>( "identifier"sv, nv ).empty() );
      REQUIRE( nv.find( "integer"sv ) != nv.cend() );
      CHECK( bsonValue<int64_t>( "integer"sv, nv ) == 0 );

      CHECK( data.find( "identifier"sv ) == data.cend() );
      CHECK( data.find( "nested"sv ) == data.cend() );
      CHECK( data.find( "nesteds"sv ) == data.cend() );
      CHECK( data.find( "strings"sv ) == data.cend() );
      CHECK( data.find( "ostring"sv ) == data.cend() );
      CHECK( data.find( "obool"sv ) == data.cend() );
      CHECK( data.find( "_id"sv ) != data.cend() );
      CHECK( data.find( "boolean"sv ) != data.cend() );

      auto copy = unmarshall<test::serial::Full>( data.view() );
      CHECK( copy.notVisitable.identifier.empty() );
      CHECK( copy.notVisitable.integer == 0 );
      CHECK( copy.identifier.empty() );
      CHECK_FALSE( copy.nested );
      CHECK( copy.nesteds.empty() );
      CHECK( copy.strings.empty() );
      CHECK_FALSE( copy.ostring );
      CHECK_FALSE( copy.obool );
      CHECK( copy.id == obj.id );
      CHECK( copy.boolean == obj.boolean );
    }

    AND_WHEN( "Serialising the struct with data" )
    {
      obj.notVisitable.identifier = "xyz-987";
      obj.notVisitable.integer = 456;
      obj.identifier = "abc-123"s;
      obj.nested = test::serial::Full::Nested{ .identifier = "nested-123"s, .integer = 1234, .number = 1.234, .date = std::chrono::system_clock::now(), .numbers = { 1.2, 2.3, 3.4 } };
      obj.nesteds = {
          test::serial::Full::Nested{ .identifier = "nested-1"s, .integer = 1, .number = 1.1, .date = std::chrono::system_clock::now(), .numbers = { 1.1, 1.2, 1.3 } },
          test::serial::Full::Nested{ .identifier = "nested-2"s, .integer = 2, .number = 2.1, .date = std::chrono::system_clock::now(), .numbers = { 2.1, 2.2, 2.3 } },
          test::serial::Full::Nested{ .identifier = "nested-3"s, .integer = 3, .number = 3.1, .date = std::chrono::system_clock::now(), .numbers = { 3.1, 3.2, 3.3 } }
      };
      obj.strings = { "one"s, "two"s, "three"s };
      obj.ostring = "some string value"s;
      obj.obool = true;
      obj.boolean = true;

      auto data = marshall( obj );

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

      REQUIRE( data.find( "strings"sv ) != data.cend() );
      auto strings = std::vector<std::string>{};
      strings.reserve( obj.strings.size() );
      for ( const auto& str : bsonValue<bsoncxx::array::view>( "strings"sv, data.view() ) ) strings.emplace_back( str.get_string().value );
      CHECK( strings == obj.strings );

      REQUIRE( data.find( "ostring"sv ) != data.cend() );
      CHECK_THAT( bsonValue<std::string>( "ostring"sv, data.view() ), Catch::Matchers::Equals( *obj.ostring ) );
      REQUIRE( data.find( "obool"sv ) != data.cend() );
      CHECK( bsonValue<bool>( "obool"sv, data.view() ) == *obj.obool );
      REQUIRE( data.find( "boolean"sv ) != data.cend() );
      CHECK( bsonValue<bool>( "boolean"sv, data.view() ) == obj.boolean );
    }
  }

  GIVEN( "A partially visitable struct" )
  {
    static_assert( !visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>(), "Partial is fully visitable" );
    REQUIRE_FALSE( visit_struct::traits::ext::is_fully_visitable<test::serial::Partial>() );
    auto obj = test::serial::Partial{};

    WHEN( "Serialising the structure with no data" )
    {
      auto data = builder( obj ) << bsoncxx::builder::stream::finalize;

      CHECK( data.find( "notVisitable"sv ) != data.cend() );
      auto nv = data["notVisitable"sv].get_document().value;
      REQUIRE( nv.find( "identifier"sv ) != nv.cend() );
      CHECK( bsonValue<std::string>( "identifier"sv, nv ).empty() );
      REQUIRE( nv.find( "integer"sv ) != nv.cend() );
      CHECK( bsonValue<int64_t>( "integer"sv, nv ) == 0 );

      CHECK( data.find( "identifier"sv ) == data.cend() );
      CHECK( data.find( "hidden"sv ) == data.cend() );
      CHECK( data.find( "_id"sv ) != data.cend() );

      auto copy = unmarshall<test::serial::Partial>( data.view() );
      CHECK( copy.identifier.empty() );
      CHECK( copy.hidden.empty() );
      CHECK( copy.id == obj.id );
    }

    AND_WHEN( "Serialising the structure with data" )
    {
      obj.notVisitable.identifier = "lmn-456";
      obj.notVisitable.integer = -1234;
      obj.identifier = "abc123"s;
      obj.hidden = "hidden text";

      auto data = builder( obj ) << bsoncxx::builder::stream::finalize;

      REQUIRE( data.find( "notVisitable"sv ) != data.cend() );
      auto nv = test::serial::NotVisitable{};
      set( nv, bsoncxx::types::bson_value::value{ bsonValue<bsoncxx::document::view>( "notVisitable"sv, data ) } );
      CHECK_THAT( nv.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
      CHECK( nv.integer == obj.notVisitable.integer );

      REQUIRE( data.find( "identifier"sv ) != data.cend() );
      CHECK_THAT( bsonValue<std::string>( "identifier"sv, data.view() ), Catch::Matchers::Equals( obj.identifier ) );
      CHECK( data.find( "hidden"sv ) == data.cend() );
      REQUIRE( data.find( "_id"sv ) != data.cend() );
      CHECK( bsonValue<bsoncxx::oid>( "_id"sv, data.view() ) == obj.id );

      auto copy = unmarshall<test::serial::Partial>( data.view() );
      CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
      CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
      CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
      CHECK( copy.hidden.empty() );
      CHECK( copy.id == obj.id );

      auto b = builder( obj );
      b << "hidden"sv << obj.hidden;
      data = b << bsoncxx::builder::stream::finalize;

      REQUIRE( data.find( "notVisitable"sv ) != data.cend() );
      nv = test::serial::NotVisitable{};
      set( nv, bsoncxx::types::bson_value::value{ bsonValue<bsoncxx::document::view>( "notVisitable"sv, data ) } );
      CHECK_THAT( nv.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
      CHECK( nv.integer == obj.notVisitable.integer );

      REQUIRE( data.find( "identifier"sv ) != data.cend() );
      CHECK_THAT( bsonValue<std::string>( "identifier"sv, data.view() ), Catch::Matchers::Equals( obj.identifier ) );
      REQUIRE( data.find( "hidden"sv ) != data.cend() );
      CHECK_THAT( bsonValue<std::string>( "hidden"sv, data.view() ), Catch::Matchers::Equals( obj.hidden ) );
      REQUIRE( data.find( "_id"sv ) != data.cend() );
      CHECK( bsonValue<bsoncxx::oid>( "_id"sv, data.view() ) == obj.id );

      copy = unmarshall<test::serial::Partial>( data.view() );
      CHECK_THAT( copy.notVisitable.identifier, Catch::Matchers::Equals( obj.notVisitable.identifier ) );
      CHECK( copy.notVisitable.integer == obj.notVisitable.integer );
      CHECK_THAT( copy.identifier, Catch::Matchers::Equals( obj.identifier ) );
      CHECK( copy.hidden.empty() );
      CHECK( copy.id == obj.id );
    }
  }
}
