//
// Created by Rakesh on 05/08/2023.
//

#include "../../src/api/api.hpp"
#include "../../src/common/util/bson.hpp"

#include <thread>
#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using std::operator""s;

namespace
{
  namespace prename
  {
    const auto oid = bsoncxx::oid{};
    std::string vhdb;
    std::string vhc;
    auto vhoid = bsoncxx::oid{};
  }
}

SCENARIO( "Rename collection test suite", "[rename]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    static const auto database = "itest"s;
    static const auto collection = "oldname"s;
    static const auto renamed = "newname"s;

    WHEN( "Document created in a new collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto request  = spt::mongoservice::api::Request::create(
          database, collection,
          basic::make_document(
              kvp( "key", "value" ), kvp( "_id", prename::oid ) )
      );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      REQUIRE( opt.find( "entity" ) != opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
      REQUIRE( opt["entity"].get_oid().value == prename::oid );

      prename::vhdb = spt::util::bsonValue<std::string>( "database", opt );
      prename::vhc = spt::util::bsonValue<std::string>( "collection", opt );
      prename::vhoid = spt::util::bsonValue<bsoncxx::oid>( "_id", opt );
    }

    THEN( "Cannot rename to existing collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto request  = spt::mongoservice::api::Request::renameCollection(
          database, collection,
          basic::make_document( kvp( "target", "test" ) )
      );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) != opt.end() );
    }

    THEN( "Rename collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto request  = spt::mongoservice::api::Request::renameCollection(
          database, collection,
          basic::make_document( kvp( "target", renamed ) )
      );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      REQUIRE( spt::util::bsonValue<std::string>( "database", opt ) == database );
      REQUIRE( spt::util::bsonValue<std::string>( "collection", opt ) == renamed );
    }

    AND_THEN( "Retrieve document from renamed collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", database ),
          kvp( "collection", renamed ),
          kvp( "document", basic::make_document( kvp( "_id", prename::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) != opt.end() );
      REQUIRE( opt.find( "results" ) == opt.end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", opt );
      REQUIRE( dv["_id"].get_oid().value == prename::oid );
      const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
      REQUIRE( key );
      REQUIRE( key == "value" );
    }

    AND_THEN( "Retrieve version history for renamed collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", prename::vhdb ),
          kvp( "collection", prename::vhc ),
          kvp( "document", basic::make_document(
              kvp( "_id", prename::vhoid ),
              kvp( "database", database ),
              kvp( "collection", renamed )
              ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) != opt.end() );
      REQUIRE( opt.find( "results" ) == opt.end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", opt );
      REQUIRE( dv["_id"].get_oid().value == prename::vhoid );

      const auto entity = spt::util::bsonValueIfExists<bsoncxx::document::view>( "entity", dv );
      REQUIRE( entity );
      REQUIRE( (*entity)["_id"].get_oid().value == prename::oid );
    }

    AND_THEN( "Drop renamed collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto request  = spt::mongoservice::api::Request::dropCollection(
          database, renamed,
          basic::make_document( kvp( "clearVersionHistory", true ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "dropCollection" ) != opt.end() );
    }

    AND_THEN( "Revision history cleared for dropped collection" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      std::this_thread::sleep_for( std::chrono::milliseconds{ 250 } );
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", prename::vhdb ),
          kvp( "collection", prename::vhc ),
          kvp( "document", basic::make_document(
              kvp( "_id", prename::vhoid ),
              kvp( "database", database ),
              kvp( "collection", renamed )
          ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[rename] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) != opt.end() );
    }
  }
}
