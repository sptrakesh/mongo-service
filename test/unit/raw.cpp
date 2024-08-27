//
// Created by Rakesh on 28/02/2024.
//

#include "../../src/common/util/json.hpp"
#include "../../src/common/util/serialise.hpp"
#include "../../src/common/visit_struct/visit_struct_intrusive.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using std::operator""sv;

namespace spt::util::test::raw
{
  struct Raw
  {
    Raw() = default;

    explicit Raw( std::string_view json )
    {
      spt::util::json::unmarshall( *this, json );
    }

    explicit Raw( bsoncxx::document::view bson )
    {
      spt::util::unmarshall( *this, bson );
    }

    BEGIN_VISITABLES(Raw);
    VISITABLE(boost::json::array, array);
    VISITABLE(boost::json::object, object);
    VISITABLE_DIRECT_INIT(bsoncxx::document::value, bson, {bsoncxx::builder::stream::document{} << bsoncxx::builder::stream::finalize});
    VISITABLE_DIRECT_INIT(bsoncxx::array::value, bsonArray, {bsoncxx::builder::stream::array{} << bsoncxx::builder::stream::finalize});
    VISITABLE(std::string, str);
    END_VISITABLES;
  };
}

SCENARIO( "Raw BSON/JSON fields test suite", "[raw]" )
{
  GIVEN( "A struct with raw BSON/JSON fields" )
  {
    const auto json = R"json({
  "str": "simple string property",
  "array": [
    "string value",
    false,
    null,
    1234.567,
    9876,
    "2021-08-06T16:05:35.423Z",
    "6024807e46ef510017f40a23",
    {
      "location": {
        "type": "Point",
        "coordinates": [
          39.29666683199505,
          -76.62490858613457
        ],
        "accuracy": 0
      }
    },
    [
      39.29666683199505,
      -76.62490858613457
    ]
  ],
  "object": {
    "id": "6024807e46ef510017f40a23",
    "country": "USA",
    "created": "2021-08-06T16:05:35.423Z"
  },
  "bson": {
    "id": "6070e60e01c47800161d68a8",
    "identifier": "NA-newdirection",
    "type": "WorkOrder"
  },
  "bsonArray": [
    "6024807e46ef510017f40a23",
    "2021-08-06T16:05:35.423Z"
  ]
})json"sv;

    WHEN( "Parsing struct from JSON" )
    {
      auto obj = spt::util::test::raw::Raw{ json };
      CHECK_THAT( obj.str, Catch::Matchers::Equals( "simple string property" ) );
      REQUIRE( obj.array.size() == 9 );
      CHECK( obj.array[0].is_string() );
      CHECK( obj.array[0].as_string() == "string value"sv );
      CHECK( obj.array[1].is_bool() );
      CHECK_FALSE( obj.array[1].as_bool() );
      CHECK( obj.array[2].is_null() );
      CHECK( obj.array[3].is_double() );
      CHECK( obj.array[3].as_double() == 1234.567 );
      CHECK( obj.array[4].is_int64() );
      CHECK( obj.array[4].as_int64() == 9876 );
      CHECK( obj.array[5].is_string() );
      CHECK( obj.array[5].as_string() == "2021-08-06T16:05:35.423Z"sv );
      CHECK( obj.array[6].is_string() );
      CHECK( obj.array[6].as_string() == "6024807e46ef510017f40a23"sv );
      CHECK( obj.array[7].is_object() );

      auto& child = obj.array[7].as_object();
      CHECK( child.contains( "location" ) );
      CHECK( child["location"].is_object() );

      auto& l = child["location"].as_object();
      CHECK( l.contains( "type" ) );
      CHECK( l["type"].is_string() );
      CHECK( l["type"].as_string() == "Point"sv );
      CHECK( l.contains( "coordinates" ) );
      CHECK( l["coordinates"].is_array() );

      auto& c = l["coordinates"].as_array();
      REQUIRE( c.size() == 2 );
      CHECK( c[0].is_double() );
      CHECK( c[0].as_double() == 39.29666683199505 );
      CHECK( c[1].is_double() );
      CHECK( c[1].as_double() == -76.62490858613457 );

      CHECK( obj.array[8].is_array() );
      c = obj.array[8].as_array();
      REQUIRE( c.size() == 2 );
      CHECK( c[0].is_double() );
      CHECK( c[0].as_double() == 39.29666683199505 );
      CHECK( c[1].is_double() );
      CHECK( c[1].as_double() == -76.62490858613457 );

      REQUIRE( obj.object.size() == 3 );
      CHECK( obj.object.contains( "id" ) );
      CHECK( obj.object["id"].is_string() );
      CHECK( obj.object["id"].as_string() == "6024807e46ef510017f40a23" );
      CHECK( obj.object.contains( "country" ) );
      CHECK( obj.object["country"].is_string() );
      CHECK( obj.object["country"].as_string() == "USA" );
      CHECK( obj.object.contains( "created" ) );
      CHECK( obj.object["created"].is_string() );
      CHECK( obj.object["created"].as_string() == "2021-08-06T16:05:35.423Z" );

      auto id = spt::util::bsonValueIfExists<bsoncxx::oid>( "id", obj.bson.view() );
      REQUIRE( id );
      CHECK( id->to_string() == "6070e60e01c47800161d68a8" );

      auto identifier = spt::util::bsonValueIfExists<std::string>( "identifier", obj.bson );
      REQUIRE( identifier );
      CHECK( *identifier == "NA-newdirection"sv );

      auto type = spt::util::bsonValueIfExists<std::string>( "type", obj.bson.view() );
      REQUIRE( type );
      CHECK( *type == "WorkOrder" );

      auto iter = obj.bsonArray.view().begin();
      REQUIRE( iter != obj.bsonArray.view().end() );
      REQUIRE( iter->type() == bsoncxx::type::k_oid );
      CHECK( iter->get_oid().value.to_string() == "6024807e46ef510017f40a23" );

      ++iter;
      REQUIRE( iter != obj.bsonArray.view().end() );
      REQUIRE( iter->type() == bsoncxx::type::k_date );
      CHECK( spt::util::isoDateMillis( iter->get_date().value ) == "2021-08-06T16:05:35.423Z" );
    }

    AND_WHEN( "Serialising struct to BSON" )
    {
      auto obj = spt::util::test::raw::Raw{ json };
      auto bson = spt::util::marshall( obj );

      auto str = spt::util::bsonValueIfExists<std::string>( "str", bson );
      REQUIRE( str );
      CHECK( *str == "simple string property"sv );

      auto arr = spt::util::bsonValueIfExists<bsoncxx::array::view>( "array", bson );
      REQUIRE( arr );

      auto aiter = arr->begin();
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_utf8 );
      CHECK( aiter->get_string().value == "string value"sv );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_bool );
      CHECK_FALSE( aiter->get_bool().value );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_double );
      REQUIRE( aiter->get_double() == 1234.567 );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_int64 );
      REQUIRE( aiter->get_int64() == 9876 );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_date );
      REQUIRE( spt::util::isoDateMillis( aiter->get_date().value ) == "2021-08-06T16:05:35.423Z" );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_oid );
      REQUIRE( aiter->get_oid().value.to_string() == "6024807e46ef510017f40a23" );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_document );

      auto doc = aiter->get_document().value;
      auto loc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "location"sv, doc );
      REQUIRE( loc );

      auto type = spt::util::bsonValueIfExists<std::string>( "type", *loc );
      REQUIRE( type );
      CHECK( *type == "Point"sv );

      auto c = spt::util::bsonValueIfExists<bsoncxx::array::view>( "coordinates", *loc );
      REQUIRE( c );

      auto citer = c->begin();
      REQUIRE( citer != c->end() );
      REQUIRE( citer->type() == bsoncxx::type::k_double );
      CHECK( citer->get_double() == 39.29666683199505 );

      ++citer;
      REQUIRE( citer != c->end() );
      REQUIRE( citer->type() == bsoncxx::type::k_double );
      CHECK( citer->get_double() == -76.62490858613457 );

      auto acc = spt::util::bsonValueIfExists<int32_t>( "accuracy", *loc );
      REQUIRE( acc );
      CHECK( *acc == 0 );

      ++aiter;
      REQUIRE( aiter != arr->end() );
      REQUIRE( aiter->type() == bsoncxx::type::k_array );
      auto co = aiter->get_array().value;

      citer = co.begin();
      REQUIRE( citer != co.end() );
      REQUIRE( citer->type() == bsoncxx::type::k_double );
      CHECK( citer->get_double() == 39.29666683199505 );

      ++citer;
      REQUIRE( citer != co.end() );
      REQUIRE( citer->type() == bsoncxx::type::k_double );
      CHECK( citer->get_double() == -76.62490858613457 );

      auto object = spt::util::bsonValueIfExists<bsoncxx::document::view>( "object", bson );
      REQUIRE( object );

      auto id = spt::util::bsonValueIfExists<bsoncxx::oid>( "id", *object );
      REQUIRE( id );
      CHECK( id->to_string() == "6024807e46ef510017f40a23" );

      auto country = spt::util::bsonValueIfExists<std::string>( "country", *object );
      REQUIRE( country );
      CHECK( *country == "USA" );

      auto created = spt::util::bsonValueIfExists<spt::util::DateTime>( "created", *object );
      REQUIRE( created );
      CHECK( spt::util::isoDateMillis( *created ) == "2021-08-06T16:05:35.423Z" );

      auto bs = spt::util::bsonValueIfExists<bsoncxx::document::view>( "bson", bson );
      REQUIRE( bs );

      id = spt::util::bsonValueIfExists<bsoncxx::oid>( "id", *bs );
      REQUIRE( id );
      CHECK( id->to_string() == "6070e60e01c47800161d68a8" );

      auto identifier = spt::util::bsonValueIfExists<std::string>( "identifier", *bs );
      REQUIRE( identifier );
      CHECK( *identifier == "NA-newdirection" );

      type = spt::util::bsonValueIfExists<std::string>( "type", *bs );
      REQUIRE( type );
      CHECK( *type == "WorkOrder" );

      arr = spt::util::bsonValueIfExists<bsoncxx::array::view>( "bsonArray", bson );
      REQUIRE( arr );

      auto iter = arr->begin();
      REQUIRE( iter != arr->end() );
      REQUIRE( iter->type() == bsoncxx::type::k_oid );
      REQUIRE( iter->get_oid().value.to_string() == "6024807e46ef510017f40a23" );

      ++iter;
      REQUIRE( iter != arr->end() );
      REQUIRE( iter->type() == bsoncxx::type::k_date );
      REQUIRE( spt::util::isoDateMillis( iter->get_date().value ) == "2021-08-06T16:05:35.423Z" );
    }

    AND_WHEN( "Parsing struct from BSON" )
    {
      auto obj1 = spt::util::test::raw::Raw{ json };
      auto obj = spt::util::test::raw::Raw{ spt::util::marshall( obj1 ) };
      CHECK_THAT( obj.str, Catch::Matchers::Equals( "simple string property" ) );
      REQUIRE( obj.array.size() == 8 );

      CHECK( obj.array[0].is_string() );
      CHECK( obj.array[0].as_string() == "string value"sv );
      CHECK( obj.array[1].is_bool() );
      CHECK_FALSE( obj.array[1].as_bool() );
      CHECK( obj.array[2].is_double() );
      CHECK( obj.array[2].as_double() == 1234.567 );
      CHECK( obj.array[3].is_int64() );
      CHECK( obj.array[3].as_int64() == 9876 );
      CHECK( obj.array[4].is_string() );
      CHECK( obj.array[4].as_string() == "2021-08-06T16:05:35.423Z"sv );
      CHECK( obj.array[5].is_string() );
      CHECK( obj.array[5].as_string() == "6024807e46ef510017f40a23"sv );
      CHECK( obj.array[6].is_object() );

      auto& child = obj.array[6].as_object();
      CHECK( child.contains( "location" ) );
      CHECK( child["location"].is_object() );

      auto& l = child["location"].as_object();
      CHECK( l.contains( "type" ) );
      CHECK( l["type"].is_string() );
      CHECK( l["type"].as_string() == "Point"sv );
      CHECK( l.contains( "coordinates" ) );
      CHECK( l["coordinates"].is_array() );

      auto& c = l["coordinates"].as_array();
      REQUIRE( c.size() == 2 );
      CHECK( c[0].is_double() );
      CHECK( c[0].as_double() == 39.29666683199505 );
      CHECK( c[1].is_double() );
      CHECK( c[1].as_double() == -76.62490858613457 );

      CHECK( obj.array[7].is_array() );
      c = obj.array[7].as_array();
      REQUIRE( c.size() == 2 );
      CHECK( c[0].is_double() );
      CHECK( c[0].as_double() == 39.29666683199505 );
      CHECK( c[1].is_double() );
      CHECK( c[1].as_double() == -76.62490858613457 );

      REQUIRE( obj.object.size() == 3 );
      CHECK( obj.object.contains( "id" ) );
      CHECK( obj.object["id"].is_string() );
      CHECK( obj.object["id"].as_string() == "6024807e46ef510017f40a23" );
      CHECK( obj.object.contains( "country" ) );
      CHECK( obj.object["country"].is_string() );
      CHECK( obj.object["country"].as_string() == "USA" );
      CHECK( obj.object.contains( "created" ) );
      CHECK( obj.object["created"].is_string() );
      CHECK( obj.object["created"].as_string() == "2021-08-06T16:05:35.423Z" );

      auto id = spt::util::bsonValueIfExists<bsoncxx::oid>( "id", obj.bson.view() );
      REQUIRE( id );
      CHECK( id->to_string() == "6070e60e01c47800161d68a8" );

      auto identifier = spt::util::bsonValueIfExists<std::string>( "identifier", obj.bson );
      REQUIRE( identifier );
      CHECK( *identifier == "NA-newdirection"sv );

      auto type = spt::util::bsonValueIfExists<std::string>( "type", obj.bson.view() );
      REQUIRE( type );
      CHECK( *type == "WorkOrder" );

      auto iter = obj.bsonArray.view().begin();
      REQUIRE( iter != obj.bsonArray.view().end() );
      REQUIRE( iter->type() == bsoncxx::type::k_oid );
      CHECK( iter->get_oid().value.to_string() == "6024807e46ef510017f40a23" );

      ++iter;
      REQUIRE( iter != obj.bsonArray.view().end() );
      REQUIRE( iter->type() == bsoncxx::type::k_date );
      CHECK( spt::util::isoDateMillis( iter->get_date().value ) == "2021-08-06T16:05:35.423Z" );
    }
  }
}
