//
// Created by Rakesh on 22/07/2020.
//
#include "../../src/api/api.hpp"
#include "../../src/common/util/bson.hpp"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace spt::itest::crud
{
  const auto oid = bsoncxx::oid{};
  std::string vhdb;
  std::string vhc;
  auto vhoid = bsoncxx::oid{};
  int64_t count = 0;
}

SCENARIO( "Simple CRUD test suite", "[crud]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    WHEN( "Creating a document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto request  = spt::mongoservice::api::Request::create(
          "itest", "test",
          basic::make_document(
              kvp( "key", "value" ), kvp( "_id", spt::itest::crud::oid ) )
          );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      REQUIRE( opt.find( "entity" ) != opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
      REQUIRE( opt["entity"].get_oid().value == spt::itest::crud::oid );

      spt::itest::crud::vhdb = spt::util::bsonValue<std::string>( "database", opt );
      spt::itest::crud::vhc = spt::util::bsonValue<std::string>( "collection", opt );
      spt::itest::crud::vhoid = spt::util::bsonValue<bsoncxx::oid>( "_id", opt );
    }

    AND_THEN( "Retrieving count of documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ),
          kvp( "skipMetric", true ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      REQUIRE( option->view().find( "error" ) == option->view().end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", option->view() );
      REQUIRE( count );
      spt::itest::crud::count = *count;
    }

    THEN( "Retrieving the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) != opt.end() );
      REQUIRE( opt.find( "results" ) == opt.end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", opt );
      REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::oid );
      const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
      REQUIRE( key );
      REQUIRE( key == "value" );
    }

    THEN( "Retrieving the document by property" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "key", "value" ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", opt );
      REQUIRE_FALSE( arr.empty() );
      bool found = false;
      for ( auto e : arr )
      {
        const auto dv = e.get_document().view();
        if ( dv["_id"].get_oid().value == spt::itest::crud::oid ) found = true;
        const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
        REQUIRE( key );
        REQUIRE( key == "value" );
      }
      REQUIRE( found );
    }

    AND_THEN( "Retrieve the version history document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::crud::vhdb ),
          kvp( "collection", spt::itest::crud::vhc ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::vhoid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) != opt.end() );
      REQUIRE( opt.find( "results" ) == opt.end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", opt );
      REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::vhoid );

      const auto entity = spt::util::bsonValueIfExists<bsoncxx::document::view>( "entity", dv );
      REQUIRE( entity );
      REQUIRE( (*entity)["_id"].get_oid().value == spt::itest::crud::oid );
    }

    AND_THEN( "Retrieve the version history document by entity id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::crud::vhdb ),
          kvp( "collection", spt::itest::crud::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::crud::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", opt );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        const auto dv = e.get_document().view();
        REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::vhoid );

        const auto ev = spt::util::bsonValue<bsoncxx::document::view>( "entity", dv );
        REQUIRE( ev["_id"].get_oid().value == spt::itest::crud::oid );
        const auto key = spt::util::bsonValueIfExists<std::string>( "key", ev );
        REQUIRE( key );
        REQUIRE( key == "value" );
        ++i;
      }
      REQUIRE( i == 1 );
    }

    AND_THEN( "Updating the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "update" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "key1", "value1" ),
              kvp( "date", bsoncxx::types::b_date{ std::chrono::system_clock::now() } ),
              kvp( "_id", spt::itest::crud::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", opt );
      REQUIRE( doc );
      REQUIRE( doc->find( "_id" ) != doc->end() );
      REQUIRE( (*doc)["_id"].get_oid().value == spt::itest::crud::oid );

      auto key = spt::util::bsonValueIfExists<std::string>( "key", *doc );
      REQUIRE( key );
      REQUIRE( *key == "value" );

      key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
      REQUIRE( key );
      REQUIRE( *key == "value1" );
    }

    AND_THEN( "Updating the document without version history" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "update" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "skipVersion", true ),
          kvp( "document", basic::make_document(
              kvp( "key1", "value1" ), kvp( "_id", spt::itest::crud::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", opt );
      REQUIRE_FALSE( doc );

      const auto skip = spt::util::bsonValueIfExists<bool>( "skipVersion", opt );
      REQUIRE( skip );
      REQUIRE( *skip );
    }

    AND_THEN( "Deleting the document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "delete" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::oid ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "success" ) != opt.end() );
      REQUIRE( opt.find( "history" ) != opt.end() );
    }

    AND_THEN( "Retriving count of documents after delete" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[crud] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", opt );
      REQUIRE( count );
      REQUIRE( spt::itest::crud::count > *count );
    }
  }
}
