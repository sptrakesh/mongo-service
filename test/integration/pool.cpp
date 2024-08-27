//
// Created by Rakesh on 12/08/2020.
//
#include "../../src/api/impl/connection.hpp"
#include "../../src/api/pool/pool.hpp"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <vector>

namespace spt::itest::pool
{
  void count( spt::mongoservice::pool::Pool<spt::mongoservice::api::impl::Connection>& pool )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "count" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document() ) );
    auto copt = pool.acquire();
    const auto option = (*copt)->execute( document.view() );
    LOG_INFO << "[pool] " << bsoncxx::to_json( *option );
  }
}

SCENARIO( "Simple connection pool test suite", "[pool]" )
{
  GIVEN( "A connection pool to Mongo Service" )
  {
    auto config = spt::mongoservice::pool::Configuration{};
    config.initialSize = 1;
    config.maxPoolSize = 3;
    config.maxConnections = 5;
    config.maxIdleTime = std::chrono::seconds{ 3 };
    spt::mongoservice::pool::Pool<spt::mongoservice::api::impl::Connection> pool{ spt::mongoservice::api::impl::create, std::move( config ) };

    WHEN( "Using initial connection to count documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );

      REQUIRE( pool.active() == 0 );
      REQUIRE( pool.inactive() == 1 );
      REQUIRE( pool.totalCreated() == 1 );
      auto copt = pool.acquire();
      REQUIRE( copt );

      const auto option = (*copt)->execute( document.view() );
      REQUIRE( option.has_value() );
      LOG_INFO << "[pool] " << bsoncxx::to_json( *option );

      const auto ov = option->view();
      REQUIRE( ov.find( "error" ) == ov.end() );
      REQUIRE( ov.find( "count" ) != ov.end() );

      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 1 );
      REQUIRE( pool.totalCreated() == 1 );
    }

    AND_WHEN( "Pool is capped by maximum" )
    {
      REQUIRE( pool.active() == 0 );
      REQUIRE( pool.inactive() == 1 );

      {
        auto copt1 = pool.acquire();
        REQUIRE( copt1 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 1 );
        REQUIRE( pool.totalCreated() == 1 );

        auto copt2 = pool.acquire();
        REQUIRE( copt2 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 2 );
        REQUIRE( pool.totalCreated() == 2 );

        auto copt3 = pool.acquire();
        REQUIRE( copt3 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 3 );
        REQUIRE( pool.totalCreated() == 3 );

        auto copt4 = pool.acquire();
        REQUIRE( copt4 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 4 );
        REQUIRE( pool.totalCreated() == 4 );

        auto copt5 = pool.acquire();
        REQUIRE( copt5 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 5 );
        REQUIRE( pool.totalCreated() == 5 );

        auto copt6 = pool.acquire();
        REQUIRE_FALSE( copt6 );
        REQUIRE( pool.inactive() == 0 );
        REQUIRE( pool.active() == 5 );
        REQUIRE( pool.totalCreated() == 5 );
      }

      REQUIRE( pool.inactive() == 3 );
      REQUIRE( pool.active() == 0 );
      REQUIRE( pool.totalCreated() == 5 );
    }

    AND_WHEN( "Reusing connections from pool" )
    {
      {
        auto copt1 = pool.acquire();
        auto copt2 = pool.acquire();
      }

      REQUIRE( pool.totalCreated() == 2 );
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );

      REQUIRE( pool.inactive() == 2 );
      REQUIRE( pool.active() == 0 );
      auto copt1 = pool.acquire();
      REQUIRE( copt1 );
      auto copt2 = pool.acquire();
      REQUIRE( copt2 );

      const auto option = (*copt2)->execute( document.view() );
      REQUIRE( option.has_value() );
      LOG_INFO << "[pool] " << bsoncxx::to_json( *option );
      const auto ov = option->view();
      REQUIRE( ov.find( "error" ) == ov.end() );
      REQUIRE( ov.find( "count" ) != ov.end() );

      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 2 );
      REQUIRE( pool.totalCreated() == 2 );
    }

    AND_WHEN( "Invalid connections are not added to pool" )
    {
      {
        auto copt1 = pool.acquire();
        (*copt1)->setValid( false );
        auto copt2 = pool.acquire();
        (*copt2)->setValid( false );
        REQUIRE( pool.active() == 2 );
      }
      REQUIRE( pool.inactive() == 0 );
    }

    AND_WHEN( "Connection TTL works" )
    {
      {
        auto copt1 = pool.acquire();
        auto copt2 = pool.acquire();
      }
      REQUIRE( pool.inactive() == 2 );
      LOG_INFO << "[pool] " << "Sleeping 6s to test TTL";
      std::this_thread::sleep_for( std::chrono::seconds{ 6 } );
      REQUIRE( pool.inactive() == 0 );
    }

    AND_WHEN( "Using pool from multiple threads" )
    {
      auto v = std::vector<std::thread>{};
      v.reserve( 5 );
      for ( int i = 0; i < 5; ++i )
      {
        v.emplace_back( spt::itest::pool::count, std::ref( pool ) );
      }

      for ( int i = 0; i < 5; ++i ) v[i].join();
      REQUIRE( pool.inactive() == 3 );
      REQUIRE( pool.active() == 0 );
    }
  }
}
