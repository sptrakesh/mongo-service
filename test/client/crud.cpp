//
// Created by Rakesh on 15/07/2021.
//

#include "status.hpp"
#include "tasks.hpp"
#include "../../src/log/NanoLog.hpp"
#include "../../src/common/util/bson.hpp"

#include <iostream>

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace spt::client::pcrud
{
  boost::asio::awaitable<bsoncxx::oid> create( Client& client, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "create" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document(
            kvp( "key", "value client" ), kvp( "_id", id ) ) ) );

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] create - " << bsoncxx::to_json( view ) << '\n';

    assert( view.find( "error" ) == view.end() );
    assert( view.find( "database" ) != view.end() );
    assert( view.find( "collection" ) != view.end() );
    assert( view.find( "entity" ) != view.end() );
    assert( view.find( "_id" ) != view.end() );
    assert( view["entity"].get_oid().value == id );

    co_return spt::util::bsonValue<bsoncxx::oid>( "_id", view );
  }

  boost::asio::awaitable<void> count( Client& client )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "count" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document() ),
        kvp( "skipMetric", true ) );

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] count - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end() );

    const auto count = spt::util::bsonValueIfExists<int64_t>( "count", view );
    assert( count );
  }

  boost::asio::awaitable<void> byId( Client& client, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "retrieve" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( "_id", id ))));

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] byId - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end() );
    assert( view.find( "result" ) != view.end() );
    assert( view.find( "results" ) == view.end() );

    const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", view );
    assert( dv["_id"].get_oid().value == id );
    const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
    assert( key );
    assert( *key == "value client" );
  }

  boost::asio::awaitable<void> byProperty( Client& client,
      std::string_view name, std::string_view value, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "retrieve" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( name, value ) ) ) );

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] byProperty - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end());
    assert( view.find( "result" ) == view.end());
    assert( view.find( "results" ) != view.end());

    const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", view );
    assert( !arr.empty() );
    bool found = false;
    for ( auto e : arr )
    {
      const auto dv = e.get_document().view();
      if ( dv["_id"].get_oid().value == id ) found = true;
      const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
      assert( key );
      assert( key == "value client" );
    }
    assert( found );
  }

  boost::asio::awaitable<void> update( Client& client, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "update" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document(
            kvp( "key1", "value1 client" ),
            kvp( "date", bsoncxx::types::b_date{
                std::chrono::system_clock::now() } ),
            kvp( "_id", id ))));

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] update - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end() );

    const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", view );
    assert( doc );
    assert( doc->find( "_id" ) != doc->end() );
    assert(( *doc )["_id"].get_oid().value == id );

    auto key = spt::util::bsonValueIfExists<std::string>( "key", *doc );
    assert( key );
    assert( *key == "value client" );

    key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
    assert( key );
    assert( *key == "value1 client" );
  }

  boost::asio::awaitable<void> remove( Client& client, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "delete" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( "_id", id ))));

    auto opt = co_await client.execute( document.view() );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] remove - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end() );
    assert( view.find( "success" ) != view.end() );
    assert( view.find( "history" ) != view.end() );
  }
}

boost::asio::awaitable<void> spt::client::crud( Client& client )
{
  using namespace std::string_view_literals;
  const auto oid = bsoncxx::oid{};

  auto vhid = co_await pcrud::create( client, oid );
  std::cout << "[coro-client] vhid - " << vhid.to_string() << '\n';
  co_await pcrud::count( client );
  co_await pcrud::byId( client, oid );
  co_await pcrud::byProperty( client, "key"sv, "value client"sv, oid );
  co_await pcrud::update( client, oid );
  co_await pcrud::remove( client, oid );
  ++Status::instance().counter;
}

