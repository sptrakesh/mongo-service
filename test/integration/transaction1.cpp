//
// Created by Rakesh on 17/07/2021.
//
#include "../../src/api/api.hpp"
#include "../../src/log/NanoLog.hpp"
#include "../../src/common/util/bson.hpp"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace spt::itest::trans1
{
  const auto oid1 = bsoncxx::oid{};
  const auto oid2 = bsoncxx::oid{};
  std::string vhdb;
  std::string vhc;
}

SCENARIO( "Transaction test suite1", "[transaction1]" )
{
  /*GIVEN( "Connected to Mongo Service" )
  {
    WHEN( "Executing a transaction" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "transaction" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "items", basic::make_array(
                  basic::make_document(
                      kvp( "action", "create" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document(
                          kvp( "key", "value1" ), kvp( "_id", spt::itest::trans1::oid1 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "create" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document(
                          kvp( "key", "value2" ), kvp( "_id", spt::itest::trans1::oid2 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "delete" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document( kvp( "_id", spt::itest::trans1::oid1 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "delete" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document( kvp( "_id", spt::itest::trans1::oid2 ) ) )
                  )
                )
             )
          ))
      );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "created" ) != opt.end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "created", opt ) == 2 );
      REQUIRE( opt.find( "updated" ) != opt.end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "updated", opt ) == 0 );
      REQUIRE( opt.find( "deleted" ) != opt.end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "deleted", opt ) == 2 );

      const auto h = spt::util::bsonValueIfExists<bsoncxx::document::view>( "history", opt );
      REQUIRE( h );

      const auto vd = spt::util::bsonValueIfExists<std::string>( "database", *h );
      REQUIRE( vd );
      spt::itest::trans1::vhdb = *vd;

      const auto vc = spt::util::bsonValueIfExists<std::string>( "collection", *h );
      REQUIRE( vc );
      spt::itest::trans1::vhc = *vc;

      const auto c = spt::util::bsonValueIfExists<bsoncxx::array::view>( "created", *h );
      const auto d = spt::util::bsonValueIfExists<bsoncxx::array::view>( "deleted", *h );

      auto num = 0;
      for ( auto&& i : *c )
      {
        REQUIRE( i.type() == bsoncxx::type::k_oid );
        ++num;
      }
      REQUIRE( num == 2 );

      num = 0;
      for ( auto&& i : *d )
      {
        REQUIRE( i.type() == bsoncxx::type::k_oid );
        ++num;
      }
      REQUIRE( num == 2 );
    }

    AND_THEN( "Retrieve the version history for first document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::trans1::vhdb ),
          kvp( "collection", spt::itest::trans1::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::trans1::oid1 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", opt );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        REQUIRE( e.type() == bsoncxx::type::k_document );
        ++i;
      }
      REQUIRE( i == 2 );
    }

    AND_THEN( "Retrieve the version history for second document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::trans1::vhdb ),
          kvp( "collection", spt::itest::trans1::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::trans1::oid2 ) ) ) );

      const auto [type, option] = spt::mongoservice::api::execute( document.view() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", opt );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        REQUIRE( e.type() == bsoncxx::type::k_document );
        ++i;
      }
      REQUIRE( i == 2 );
    }
  }*/
}
