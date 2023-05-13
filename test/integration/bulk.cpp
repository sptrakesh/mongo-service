//
// Created by Rakesh on 23/09/2020.
//
#include "../../src/api/api.h"
#include "../../src/log/NanoLog.h"
#include "../../src/common/util/bson.h"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <chrono>
#include <vector>

namespace spt::itest::bulk
{
  const auto oid1 = bsoncxx::oid{};
  const auto oid2 = bsoncxx::oid{};
  std::vector<bsoncxx::oid> oids;
  int64_t count = 0;
}

SCENARIO( "Bulk operation test suite", "[bulk]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    WHEN( "Creating documents in bulk" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      auto doc = document{} <<
          "action" << "bulk" <<
          "database" << "itest" <<
          "collection" << "test" <<
          "document" <<
            open_document <<
              "insert" <<
                open_array <<
                  open_document <<
                    "_id" << spt::itest::bulk::oid1 <<
                    "key" << "value1" <<
                  close_document <<
                  open_document <<
                    "_id" << spt::itest::bulk::oid2 <<
                    "key" << "value2" <<
                  close_document <<
                close_array <<
            close_document <<
          finalize;
      LOG_INFO << "[bulk] " << bsoncxx::to_json( doc.view() );

      const auto [type, option] = spt::mongoservice::api::execute( doc.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "create" ) != opt.end() );
      REQUIRE( opt.find( "history" ) != opt.end() );
      REQUIRE( opt.find( "delete" ) != opt.end() );
    }

    AND_THEN( "Retriving count of documents" )
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
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      REQUIRE( option->view().find( "error" ) == option->view().end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", option->view() );
      REQUIRE( count );
      spt::itest::bulk::count = *count;
    }

    AND_THEN( "Deleting documents in bulk" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      auto doc = document{} <<
        "action" << "bulk" <<
        "database" << "itest" <<
        "collection" << "test" <<
        "document" <<
          open_document <<
            "delete" <<
              open_array <<
                open_document << "_id" << spt::itest::bulk::oid1 << close_document <<
                open_document << "_id" << spt::itest::bulk::oid2 << close_document <<
              close_array <<
          close_document <<
        finalize;

      const auto [type, option] = spt::mongoservice::api::execute( doc.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value());
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "create" ) != opt.end() );
      REQUIRE( opt.find( "history" ) != opt.end() );
      REQUIRE( opt.find( "delete" ) != opt.end() );
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
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      REQUIRE( option->view().find( "error" ) == option->view().end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", option->view() );
      REQUIRE( count );
      REQUIRE( spt::itest::bulk::count > *count );
    }

    AND_THEN( "Creating a large batch of documents" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto size = 10000;
      spt::itest::bulk::oids.reserve( size );
      auto arr = basic::array{};
      for ( auto i = 0; i < size; ++i )
      {
        spt::itest::bulk::oids.emplace_back( bsoncxx::oid{} );
        arr.append(
            document{} <<
              "_id" << spt::itest::bulk::oids.back() <<
              "iter" << i <<
              "key1" << "value1" <<
              "key2" << "value2" <<
              "key3" << "value3" <<
              "key4" << "value4" <<
              "key5" << "value5" <<
              "sub1" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
              "sub2" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
              "sub3" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
            finalize
            );
      }

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "bulk" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "insert", arr.extract() ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "create" ) != opt.end() );
      REQUIRE( opt.find( "history" ) != opt.end() );
      REQUIRE( opt.find( "delete" ) != opt.end() );
    }

    AND_THEN( "Deleting a large batch of documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      auto arr = basic::array{};
      for ( auto& id : spt::itest::bulk::oids )
      {
        arr.append( basic::make_document( kvp( "_id", id ) ) );
      }

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "bulk" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "delete", arr.view() ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[bulk] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "create" ) != opt.end() );
      REQUIRE( opt.find( "history" ) != opt.end() );
      REQUIRE( opt.find( "delete" ) != opt.end() );
    }
  }
}

