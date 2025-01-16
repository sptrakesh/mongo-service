//
// Created by Rakesh on 28/07/2020.
//
#include "../../src/api/api.hpp"
#include "../../src/log/NanoLog.hpp"
#include "../../src/common/util/bson.hpp"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

SCENARIO( "Index test suite", "[index]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    WHEN( "Creating an index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "unused", 1 ) ) ),
          kvp( "options", basic::make_document( kvp( "version", 2 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", opt );
      REQUIRE( name );
    }

    AND_THEN( "Creating the index again" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused", 1 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", opt );
      REQUIRE_FALSE( name );
    }

    AND_THEN( "Dropping the index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "dropIndex" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "specification", basic::make_document( kvp( "unused", 1 ) ) ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->view().find( "error" ) == option->view().end() );
    }

#if !(defined(_WIN32) || defined(WIN32))
    WHEN( "Creating a unique index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;
      using std::operator""sv;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused1", 1 ) ) ),
          kvp( "options", basic::make_document(
              kvp( "name", "uniqueIndex"sv ),
              kvp( "unique", true ),
              kvp( "expireAfterSeconds", 5 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", opt );
      REQUIRE( name );
    }

    AND_THEN( "Creating a unique index again" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;
      using std::operator""sv;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused1", 1 ) ) ),
          kvp( "options", basic::make_document(
              kvp( "name", "uniqueIndex"sv ),
              kvp( "unique", true ),
              kvp( "expireAfterSeconds", 5 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", opt );
      REQUIRE_FALSE( name );
    }

    AND_THEN( "Dropping the unique index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;
      using std::operator""sv;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "dropIndex" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "name", "uniqueIndex"sv ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->view().find( "error" ) == option->view().end() );
    }
#endif
  }
}
