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
#include <vector>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::pool
{
  struct InitLogger
  {
    static InitLogger& instance()
    {
      static InitLogger lg;
      return lg;
    }

    ~InitLogger() = default;

  private:
    InitLogger()
    {
      nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp", "pool-test", true );
    }
  };

  struct Connection
  {
    Connection( boost::asio::io_context& ioc, std::string_view h, std::string_view p ) :
        s{ ioc }, resolver( ioc ),
        host{ h.data(), h.size() }, port{ p.data(), p.size() }
    {
      boost::asio::connect( s, resolver.resolve( host, port ) );
    }

    Connection( const Connection& ) = delete;
    Connection& operator=( const Connection& ) = delete;

    ~Connection()
    {
      s.close();
    }

    std::optional<bsoncxx::document::value> execute(
        const bsoncxx::document::view_or_value& document, std::size_t bufSize = 4096 )
    {
      std::ostream os{ &buffer };
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = socket().send( buffer.data() );
      buffer.consume( isize );

      const auto documentSize = [this]( std::size_t length )
      {
        if ( length < 5 ) return length;

        const auto data = reinterpret_cast<const uint8_t*>( buffer.data().data() );
        uint32_t len;
        memcpy( &len, data, sizeof(len) );
        return std::size_t( len );
      };

      auto osize = socket().receive( buffer.prepare( bufSize ) );
      buffer.commit( osize );
      std::size_t read = osize;

      const auto docSize = documentSize( osize );
      while ( read != docSize )
      {
        osize = socket().receive( buffer.prepare( bufSize ) );
        buffer.commit( osize );
        read += osize;
      }

      const auto option = bsoncxx::validate(
          reinterpret_cast<const uint8_t*>( buffer.data().data() ), docSize );
      buffer.consume( buffer.size() );

      if ( !option ) return std::nullopt;
      return bsoncxx::document::value{ *option };
    }

    tcp::socket& socket()
    {
      if ( ! s.is_open() ) boost::asio::connect( s, resolver.resolve( host, port ) );
      return s;
    }

    bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    tcp::socket s;
    tcp::resolver resolver;
    boost::asio::streambuf buffer;
    std::string host;
    std::string port;
    bool v{ true };
  };

  std::unique_ptr<Connection> create()
  {
    static boost::asio::io_context ioc;
    return std::make_unique<Connection>( ioc, "localhost", "2020" );
  }

  void count( spt::pool::Pool<Connection>& pool )
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
    std::cout << bsoncxx::to_json( *option ) << '\n';
  }
}

SCENARIO( "Simple connection pool test suite", "[pool]" )
{
  spt::itest::pool::InitLogger::instance();

  GIVEN( "A connection pool to Mongo Service" )
  {
    auto config = spt::pool::Configuration{};
    config.initialSize = 1;
    config.maxPoolSize = 3;
    config.maxConnections = 5;
    config.maxIdleTime = std::chrono::seconds{ 3 };
    spt::pool::Pool<spt::itest::pool::Connection> pool{ spt::itest::pool::create, std::move( config ) };

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
      std::cout << bsoncxx::to_json( *option ) << '\n';

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
      std::cout << bsoncxx::to_json( *option ) << '\n';
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
      std::cout << "Sleeping 6s to test TTL" << std::endl;
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
