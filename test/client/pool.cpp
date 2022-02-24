//
// Created by Rakesh on 16/07/2021.
//

#include "status.h"
#include "tasks.h"
#include "../../src/api/contextholder.h"
#include "../../src/api/pool/pool.h"
#include "../../src/common/util/bson.h"

#include <boost/asio/co_spawn.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using spt::mongoservice::pool::Pool;

namespace spt::client::pool
{
  boost::asio::awaitable<bsoncxx::oid> create( Pool<Client>& pool, int i, std::string_view value )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::oid id;
    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "create" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document(
            kvp( "key", value ), kvp( "_id", id ) ) ) );

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " create - " << bsoncxx::to_json( view );

    assert( view.find( "error" ) == view.end() );
    assert( view.find( "database" ) != view.end() );
    assert( view.find( "collection" ) != view.end() );
    assert( view.find( "entity" ) != view.end() );
    assert( view.find( "_id" ) != view.end() );
    assert( view["entity"].get_oid().value == id );

    co_return id;
  }

  boost::asio::awaitable<void> count( Pool<Client>& pool, int i )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "count" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document()));

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " count - " << bsoncxx::to_json( view );
    assert( view.find( "error" ) == view.end());

    const auto count = spt::util::bsonValueIfExists<int64_t>( "count", view );
    assert( count );
  }

  boost::asio::awaitable<void> byId( Pool<Client>& pool, bsoncxx::oid id, int i, std::string_view value )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "retrieve" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( "_id", id ))));

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " byId - " << bsoncxx::to_json( view );
    assert( view.find( "error" ) == view.end());
    assert( view.find( "result" ) != view.end());
    assert( view.find( "results" ) == view.end());

    const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", view );
    assert( dv["_id"].get_oid().value == id );
    const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
    assert( key );
    assert( *key == value );
  }

  boost::asio::awaitable<void> byProperty( Pool<Client>& pool,
      std::string_view name, std::string_view value, bsoncxx::oid id, int i )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "retrieve" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( name, value ))));

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " byProperty - " << bsoncxx::to_json( view );
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
      assert( key == value );
    }
    assert( found );
  }

  boost::asio::awaitable<void> update( Pool<Client>& pool, bsoncxx::oid id, int i, std::string_view value )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "update" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document(
            kvp( "key1", "value1" ),
            kvp( "date", bsoncxx::types::b_date{
                std::chrono::system_clock::now() } ),
            kvp( "_id", id ))));

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " update - " << bsoncxx::to_json( view );
    assert( view.find( "error" ) == view.end() );

    const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", view );
    assert( doc );
    assert( doc->find( "_id" ) != doc->end() );
    assert(( *doc )["_id"].get_oid().value == id );

    auto key = spt::util::bsonValueIfExists<std::string>( "key", *doc );
    assert( key );
    assert( *key == value );

    key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
    assert( key );
    assert( *key == "value1" );
  }

  boost::asio::awaitable<void> remove( Pool<Client>& pool, bsoncxx::oid id, int i )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    bsoncxx::document::value document = basic::make_document(
        kvp( "action", "delete" ),
        kvp( "database", "itest" ),
        kvp( "collection", "test" ),
        kvp( "document", basic::make_document( kvp( "_id", id ))));

    auto copt = pool.acquire();
    assert( copt );
    auto opt = co_await (*copt)->execute( document.view() );
    assert( opt );

    auto view = opt->view();
    LOG_INFO << "[pool-client]" << i << " remove - " << bsoncxx::to_json( view );
    assert( view.find( "error" ) == view.end() );
    assert( view.find( "success" ) != view.end() );
    assert( view.find( "history" ) != view.end() );
  }

  boost::asio::awaitable<bool> crud( Pool<Client>& pool, int i )
  {
    using namespace std::string_view_literals;

    std::string value{};
    value.reserve( 12 );
    value.append( "value " ).append( std::to_string( i ) );

    auto id = co_await pool::create( pool, i, value );
    LOG_INFO << "[pool-client]" << i << " id - " << id.to_string();

    co_await count( pool, i );
    co_await byId( pool, id, i, value );
    co_await byProperty( pool, "key"sv, value, id, i );
    co_await update( pool, id, i, value );
    co_await remove( pool, id, i );
    co_return true;
  }
}

boost::asio::awaitable<void> spt::client::crud()
{
  auto config = mongoservice::pool::Configuration{};
  config.initialSize = 1;
  config.maxPoolSize = 10;
  config.maxConnections = 1000;
  config.maxIdleTime = std::chrono::seconds{ 3 };
  Pool<Client> pool{ createClient, std::move( config ) };
  auto& ctx = mongoservice::api::ContextHolder::instance();

  for ( int i = 0; i < 100; ++i )
  {
    LOG_INFO << "Spawning crud operations " << i;
    boost::asio::co_spawn( ctx.ioc, pool::crud( pool, i ),
        [](std::exception_ptr e, bool) { ++Status::instance().counter; } );
  }

  while ( Status::instance().counter.load() < 101 ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
  LOG_INFO << "Finished multi-threaded crud operations";
  ctx.ioc.stop();
  co_return;
}

