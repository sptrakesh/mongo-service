//
// Created by Rakesh on 14/07/2021.
//

#include "catch.hpp"
#include "coro_io.hpp"
#include "../../src/log/NanoLog.h"
#include "../../src/util/bson.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <iostream>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::coroclient
{
  template <typename Response>
  struct Result
  {
    struct promise_type
    {
      Response value;

      Result get_return_object()
      {
        return { cons::coroutine_handle<promise_type>::from_promise( *this ) };
      }

      cons::suspend_never initial_suspend() noexcept { return {}; }
      cons::suspend_always final_suspend() noexcept { return {}; }

      void unhandled_exception()
      {
        LOG_WARN << "Unhandled exception in coroutine";
      }

      cons::suspend_never return_value( Response&& r )
      {
        value = std::move( r );
        LOG_DEBUG << "Setting value " << value.has_value();
        return {};
      }
    };

    using Handle = cons::coroutine_handle<promise_type>;
    Result( Handle handle ) : h{ handle } {}
    ~Result() { if ( h ) h.destroy(); }

    Result(const Result&) = delete;
    Result( Result&& ro ) : h{ ro.h } { ro.h = nullptr; }

    bool resume()
    {
      if ( not h.done() ) h.resume();
      return not h.done();
    }

    explicit operator bool()
    {
      return not h.done();
    }

    Handle handle() const { return h; }
    const Response& value() const { return h.promise().value; }
  private:
    Handle h{ nullptr };
  };

  using OptionalValue = Result<std::optional<bsoncxx::document::value>>;

  struct Connection
  {
    Connection( boost::asio::io_context& ioc, std::string_view h,
        std::string_view p, boost::system::error_code& ec ) :
        s{ ioc }, resolver( ioc ),
        host{ h.data(), h.size() }, port{ p.data(), p.size() }
    {
      resolver.resolve( host, port, ec );
      if ( ec ) LOG_WARN << "Error resolving service " << ec.message();

      connect( ec );
      if ( ec ) LOG_WARN << "Error opening socket connection " << ec.message();
      else LOG_INFO << "Opened socket connection to " << h << ':' << p;
    }

    Connection( const Connection& ) = delete;
    Connection& operator=( const Connection& ) = delete;

    ~Connection()
    {
      boost::system::error_code ec;
      s.close( ec );

      if ( ec ) LOG_WARN << "Error closing socket " << ec.message();
    }

    Result<std::optional<bsoncxx::document::value>> execute(
        bsoncxx::document::view view, std::size_t bufSize,
        uint32_t timeoutSeconds )
    {
      LOG_INFO << "Request BSON size: " << int(view.length()) << " bytes " << timeoutSeconds;
      auto [werr, isize] = co_await coro_io::async_write(
          socket(), boost::asio::buffer( view.data(), view.length() ) );
      if ( werr )
      {
        LOG_WARN << "Error writing to socket " << werr.message();
        co_return std::nullopt;
      }
      LOG_DEBUG << "Wrote " << int(isize) << " bytes to socket";

      co_await cons::suspend_always{};
      std::vector<uint8_t> rbuf;
      rbuf.reserve( bufSize );

      const auto documentSize = [&rbuf]( std::size_t length )
      {
        if ( length < 5 ) return length;

        const auto data = reinterpret_cast<const uint8_t*>( rbuf.data() );
        uint32_t len;
        memcpy( &len, data, sizeof(len) );
        return std::size_t( len );
      };

      auto[rerr, osize] = co_await coro_io::async_read( socket(), boost::asio::buffer( rbuf ) );
      if ( rerr )
      {
        LOG_WARN << "Error reading from socket " << rerr.message();
        co_return std::nullopt;
      }
      std::size_t read = osize;

      const auto docSize = documentSize( osize );
      LOG_DEBUG << "Read " << int(read) << " bytes of " << int(docSize) << " total";
      while ( read != docSize )
      {
        co_await cons::suspend_always{};
        auto[rerr, osize] = co_await coro_io::async_read( socket(), boost::asio::buffer( rbuf ) );
        if ( rerr )
        {
          LOG_WARN << "Error reading from socket " << rerr.message();
          co_return std::nullopt;
        }
        read += osize;
      }

      const auto option = bsoncxx::validate(
          reinterpret_cast<const uint8_t*>( rbuf.data() ), docSize );

      if ( !option )
      {
        LOG_WARN << "Invalid BSON data";
        co_return std::nullopt;
      }
      co_return bsoncxx::document::value{ *option };
    }

    tcp::socket& socket()
    {
      if ( ! s.is_open() )
      {
        boost::system::error_code ec;
        connect( ec );
        if ( ec ) LOG_WARN << "Error opening socket " << ec.message();
      }
      return s;
    }

    bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    void connect( boost::system::error_code ec )
    {
      auto eps = resolver.resolve( host, port, ec );
      if ( !ec )
      {
        boost::system::error_code e;
        for ( auto&& ep : eps )
        {
          auto err = co_await coro_io::async_connect( s, ep.endpoint() );
          if ( !err ) break;
          e = err;
        }

        ec = e;
      }
    }

    tcp::socket s;
    tcp::resolver resolver;
    std::string host;
    std::string port;
    bool v{ true };
  };

  void count( Connection& con )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "count" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document() ) );

    auto coro = con.execute( document.view(), 4096, 2 );
    while ( coro ) { coro.resume(); }
    const auto& value = coro.value();

    if ( value )
    {
      std::cout << bsoncxx::to_json( *value ) << '\n';
    }
  }

  const auto oid = bsoncxx::oid{};
  std::string vhdb;
  std::string vhc;
  auto vhoid = bsoncxx::oid{};
  int64_t total = 0;
}

SCENARIO( "Simple CRUD test suite using coroutine client", "[coro-client]" )
{
  boost::asio::io_context ioc;
  std::thread io_thread{ [&]() { ioc.run(); } };

  GIVEN( "A Connection to Mongo Service" )
  {
    boost::system::error_code ec;
    spt::itest::coroclient::Connection con{ ioc, "localhost", "2020", ec };
    REQUIRE( !ec );

    WHEN( "Creating a document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "create" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document(
                kvp( "key", "value" ), kvp( "_id", spt::itest::coroclient::oid ) ) ) );

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro ) { coro.resume(); }
        REQUIRE( coro.value().has_value() );

        auto view = coro.value()->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end() );
        REQUIRE( view.find( "database" ) != view.end() );
        REQUIRE( view.find( "collection" ) != view.end() );
        REQUIRE( view.find( "entity" ) != view.end() );
        REQUIRE( view.find( "_id" ) != view.end() );
        REQUIRE( view["entity"].get_oid().value == spt::itest::coroclient::oid );

        spt::itest::coroclient::vhdb = spt::util::bsonValue<std::string>( "database", view );
        spt::itest::coroclient::vhc = spt::util::bsonValue<std::string>( "collection", view );
        spt::itest::coroclient::vhoid = spt::util::bsonValue<bsoncxx::oid>( "_id", view );
      });
    }

    AND_THEN( "Retrieving count of documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "count" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document()));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());

        const auto count = spt::util::bsonValueIfExists<int64_t>( "count",
            view );
        REQUIRE( count );
        spt::itest::coroclient::total = *count;
      });
    }

    THEN( "Retrieving the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "retrieve" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document(
                kvp( "_id", spt::itest::coroclient::oid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());
        REQUIRE( view.find( "result" ) != view.end());
        REQUIRE( view.find( "results" ) == view.end());

        const auto dv = spt::util::bsonValue<bsoncxx::document::view>(
            "result", view );
        REQUIRE( dv["_id"].get_oid().value == spt::itest::coroclient::oid );
        const auto key = spt::util::bsonValueIfExists<std::string>( "key",
            dv );
        REQUIRE( key );
        REQUIRE( key == "value" );
      });
    }

    THEN( "Retrieving the document by property" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "retrieve" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document( kvp( "key", "value" ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());
        REQUIRE( view.find( "result" ) == view.end());
        REQUIRE( view.find( "results" ) != view.end());

        const auto arr = spt::util::bsonValue<bsoncxx::array::view>(
            "results", view );
        REQUIRE_FALSE( arr.empty());
        bool found = false;
        for ( auto e : arr )
        {
          const auto dv = e.get_document().view();
          if ( dv["_id"].get_oid().value ==
              spt::itest::coroclient::oid )
            found = true;
          const auto key = spt::util::bsonValueIfExists<std::string>( "key",
              dv );
          REQUIRE( key );
          REQUIRE( key == "value" );
        }
        REQUIRE( found );
      });
    }

    AND_THEN( "Retrieve the version history document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "retrieve" ),
            kvp( "database", spt::itest::coroclient::vhdb ),
            kvp( "collection", spt::itest::coroclient::vhc ),
            kvp( "document", basic::make_document(
                kvp( "_id", spt::itest::coroclient::vhoid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());
        REQUIRE( view.find( "result" ) != view.end());
        REQUIRE( view.find( "results" ) == view.end());

        const auto dv = spt::util::bsonValue<bsoncxx::document::view>(
            "result", view );
        REQUIRE( dv["_id"].get_oid().value == spt::itest::coroclient::vhoid );

        const auto entity = spt::util::bsonValueIfExists<bsoncxx::document::view>(
            "entity", dv );
        REQUIRE( entity );
        REQUIRE(( *entity )["_id"].get_oid().value ==
            spt::itest::coroclient::oid );
      });
    }

    AND_THEN( "Retrieve the version history document by entity id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "retrieve" ),
            kvp( "database", spt::itest::coroclient::vhdb ),
            kvp( "collection", spt::itest::coroclient::vhc ),
            kvp( "document", basic::make_document(
                kvp( "entity._id", spt::itest::coroclient::oid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());
        REQUIRE( view.find( "result" ) == view.end());
        REQUIRE( view.find( "results" ) != view.end());

        const auto arr = spt::util::bsonValue<bsoncxx::array::view>(
            "results", view );
        REQUIRE_FALSE( arr.empty());

        auto i = 0;
        for ( auto e : arr )
        {
          const auto dv = e.get_document().view();
          REQUIRE(
              dv["_id"].get_oid().value == spt::itest::coroclient::vhoid );

          const auto ev = spt::util::bsonValue<bsoncxx::document::view>(
              "entity", dv );
          REQUIRE( ev["_id"].get_oid().value == spt::itest::coroclient::oid );
          const auto key = spt::util::bsonValueIfExists<std::string>( "key",
              ev );
          REQUIRE( key );
          REQUIRE( key == "value" );
          ++i;
        }
        REQUIRE( i == 1 );
      });
    }

    AND_THEN( "Updating the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "update" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document(
                kvp( "key1", "value1" ),
                kvp( "date", bsoncxx::types::b_date{
                    std::chrono::system_clock::now() } ),
                kvp( "_id", spt::itest::coroclient::oid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());

        const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>(
            "document", view );
        REQUIRE( doc );
        REQUIRE( doc->find( "_id" ) != doc->end());
        REQUIRE(
            ( *doc )["_id"].get_oid().value == spt::itest::coroclient::oid );

        auto key = spt::util::bsonValueIfExists<std::string>( "key", *doc );
        REQUIRE( key );
        REQUIRE( *key == "value" );

        key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
        REQUIRE( key );
        REQUIRE( *key == "value1" );
      });
    }

    AND_THEN( "Updating the document without version history" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "update" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "skipVersion", true ),
            kvp( "document", basic::make_document(
                kvp( "key1", "value1" ),
                kvp( "_id", spt::itest::coroclient::oid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());

        const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>(
            "document", view );
        REQUIRE_FALSE( doc );

        const auto skip = spt::util::bsonValueIfExists<bool>( "skipVersion",
            view );
        REQUIRE( skip );
        REQUIRE( *skip );
      });
    }

    AND_THEN( "Deleting the document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "delete" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document(
                kvp( "_id", spt::itest::coroclient::oid ))));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());
        REQUIRE( view.find( "success" ) != view.end());
        REQUIRE( view.find( "history" ) != view.end());
      });
    }

    AND_THEN( "Retrieving count of documents after delete" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      ioc.post([&]()
      {
        bsoncxx::document::value document = basic::make_document(
            kvp( "action", "count" ),
            kvp( "database", "itest" ),
            kvp( "collection", "test" ),
            kvp( "document", basic::make_document()));

        auto coro = con.execute( document.view(), 4096, 2 );
        while ( coro )
        { coro.resume(); }
        auto handle = coro.handle();
        REQUIRE( handle.promise().value );

        auto view = handle.promise().value->view();
        std::cout << "[coro-client]" << bsoncxx::to_json( view ) << '\n';
        REQUIRE( view.find( "error" ) == view.end());

        const auto count = spt::util::bsonValueIfExists<int64_t>( "count", view );
        REQUIRE( count );
        REQUIRE( spt::itest::coroclient::total > *count );
      });
    }
  }

  ioc.stop();
  if ( io_thread.joinable() ) io_thread.join();
}
