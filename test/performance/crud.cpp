//
// Created by Rakesh on 06/08/2020.
//

#include <hayai.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/validate.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;

struct SocketClient
{
  static int failures;

  explicit SocketClient( boost::asio::io_context& ioc ) : s{ ioc }, resolver{ ioc }
  {
    boost::asio::connect( s, resolver.resolve( "localhost", "2000" ) );
  }

  ~SocketClient()
  {
    s.close();
  }

  void run()
  {
    create();
    retrieve();
    update();
    remove();
  }

private:
  void create()
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    boost::asio::streambuf buffer;
    std::ostream os{ &buffer };
    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "create" ),
        kvp( "database", "itest" ),
        kvp( "collection", "ptest" ),
        kvp( "document", basic::make_document(
            kvp( "time", bsoncxx::types::b_date{ std::chrono::system_clock::now() } ),
            kvp( "_id", id ) ) ) );
    os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

    const auto isize = s.send( buffer.data() );
    buffer.consume( isize );

    const auto osize = s.receive( buffer.prepare( 1024 ) );
    buffer.commit( osize );

    const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
    if ( !option ) ++failures;
    else
    {
      if ( option->find( "error" ) != option->end() ) ++failures;
    }
  }

  void retrieve()
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    boost::asio::streambuf buffer;
    std::ostream os{ &buffer };
    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "retrieve" ),
        kvp( "database", "itest" ),
        kvp( "collection", "ptest" ),
        kvp( "document", basic::make_document( kvp( "_id", id ) ) ) );
    os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

    const auto isize = s.send( buffer.data() );
    buffer.consume( isize );

    const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
    buffer.commit( osize );

    const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
    if ( !option ) ++failures;
    else
    {
      if ( option->find( "error" ) != option->end() ) ++failures;
    }
  }

  void update()
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    boost::asio::streambuf buffer;
    std::ostream os{ &buffer };
    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "update" ),
        kvp( "database", "itest" ),
        kvp( "collection", "ptest" ),
        kvp( "document", basic::make_document(
            kvp( "time", bsoncxx::types::b_date{ std::chrono::system_clock::now() } ),
            kvp( "_id", id ) ) ) );
    os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

    const auto isize = s.send( buffer.data() );
    buffer.consume( isize );

    const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
    buffer.commit( osize );

    const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
    if ( !option ) ++failures;
    else
    {
      if ( option->find( "error" ) != option->end() ) ++failures;
    }
  }

  void remove()
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    boost::asio::streambuf buffer;
    std::ostream os{ &buffer };
    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "delete" ),
        kvp( "database", "itest" ),
        kvp( "collection", "ptest" ),
        kvp( "document", basic::make_document( kvp( "_id", id ) ) ) );
    os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

    const auto isize = s.send( buffer.data() );
    buffer.consume( isize );

    const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
    buffer.commit( osize );

    const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
    if ( !option ) ++failures;
    else
    {
      if ( option->find( "error" ) != option->end() ) ++failures;
    }
  }

  tcp::socket s;
  tcp::resolver resolver;
  bsoncxx::oid id;
};

int SocketClient::failures = 0;

struct SocketClientFixture : hayai::Fixture
{
  void SetUp() override
  {
    SocketClient::failures = 0;
  }

  void TearDown() override
  {
    clients.clear();
    if ( SocketClient::failures > 0 )
    {
      std::cerr << "Total failures " << SocketClient::failures << '\n';
    }
  }

  void run( int concurrency )
  {
    bool create = false;
    if ( clients.empty() )
    {
      clients.reserve( concurrency );
      create = true;
    }

    std::vector<std::thread> threads;
    threads.reserve( concurrency );
    for ( auto i = 0; i < concurrency; ++i )
    {
      if (create) clients.emplace_back( std::make_unique<SocketClient>( ioc ) );
      auto client = clients[i].get();
      threads.emplace_back( std::thread([=]
      {
        client->run();
      }));
    }

    for (auto& t : threads) t.join();
  }

  std::vector<std::unique_ptr<SocketClient>> clients;
  boost::asio::io_context ioc{ 8 };
};

#define SOCKET_TEST crud

BENCHMARK_P_F(SocketClientFixture, SOCKET_TEST, 2, 10, (int concurrency))
{
  run( concurrency );
}

BENCHMARK_P_INSTANCE(SocketClientFixture, SOCKET_TEST, (10));
BENCHMARK_P_INSTANCE(SocketClientFixture, SOCKET_TEST, (50));
BENCHMARK_P_INSTANCE(SocketClientFixture, SOCKET_TEST, (100));
BENCHMARK_P_INSTANCE(SocketClientFixture, SOCKET_TEST, (500));
BENCHMARK_P_INSTANCE(SocketClientFixture, SOCKET_TEST, (1000));
