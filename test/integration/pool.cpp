//
// Created by Rakesh on 12/08/2020.
//
#include "catch.hpp"
#include "pool.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::pool
{
  struct Connection
  {
    Connection( boost::asio::io_context& ioc, std::string_view host, std::string_view port ) :
        s{ ioc }
    {
      tcp::resolver resolver( ioc );
      boost::asio::connect( s, resolver.resolve( host, port ) );
    }

    Connection( const Connection& ) = delete;
    Connection& operator=( const Connection& ) = delete;

    ~Connection()
    {
      s.close();
    }

    tcp::socket& socket() { return s; }

  private:
    tcp::socket s;
  };

  std::unique_ptr<Connection> create()
  {
    static boost::asio::io_context ioc;
    return std::make_unique<Connection>( ioc, "localhost", "2020" );
  }
}

SCENARIO( "Simple connection pool test suite", "[pool]" )
{
  GIVEN( "A connection pool to Mongo Service" )
  {
    spt::pool::Pool<spt::itest::pool::Connection> pool{ spt::itest::pool::create, 1, 5 };

    WHEN( "Using initial connection to count documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      REQUIRE( pool.active() == 0 );
      REQUIRE( pool.inactive() == 1 );
      auto copt = pool.acquire();
      REQUIRE( copt );
      const auto isize = (*copt)->socket().send( buffer.data() );
      buffer.consume( isize );

      const auto osize = (*copt)->socket().receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "count" ) != option->end() );

      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 1 );
    }

    AND_WHEN( "Pool is capped by maximum" )
    {
      REQUIRE( pool.active() == 0 );
      REQUIRE( pool.inactive() == 1 );
      auto copt1 = pool.acquire();
      REQUIRE( copt1 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 1 );

      auto copt2 = pool.acquire();
      REQUIRE( copt2 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 2 );

      auto copt3 = pool.acquire();
      REQUIRE( copt3 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 3 );

      auto copt4 = pool.acquire();
      REQUIRE( copt4 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 4 );

      auto copt5 = pool.acquire();
      REQUIRE( copt5 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 5 );

      auto copt6 = pool.acquire();
      REQUIRE_FALSE( copt6 );
      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 5 );
    }

    AND_WHEN( "Reusing connections from pool" )
    {
      {
        auto copt1 = pool.acquire();
        auto copt2 = pool.acquire();
      }

      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      REQUIRE( pool.inactive() == 2 );
      REQUIRE( pool.active() == 0 );
      auto copt1 = pool.acquire();
      REQUIRE( copt1 );
      auto copt2 = pool.acquire();
      REQUIRE( copt2 );
      const auto isize = (*copt2)->socket().send( buffer.data() );
      buffer.consume( isize );

      const auto osize = (*copt2)->socket().receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "count" ) != option->end() );

      REQUIRE( pool.inactive() == 0 );
      REQUIRE( pool.active() == 2 );
    }
  }
}
