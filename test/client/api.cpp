//
// Created by Rakesh on 24/02/2022.
//

#include "status.h"
#include "tasks.h"
#include "../../src/api/api.h"
#include "../../src/common/util/bson.h"

#include <iostream>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace spt::client::papi
{
  boost::asio::awaitable<bsoncxx::oid> create( bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::create( "itest", "test",
        basic::make_document( kvp( "key", "value api" ), kvp( "_id", id ) ) );

    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
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

  boost::asio::awaitable<void> count()
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::count( "itest", "test", basic::make_document() );
    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] count - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end());

    const auto count = spt::util::bsonValueIfExists<int64_t>( "count", view );
    assert( count );
  }

  boost::asio::awaitable<void> byId( bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::retrieve( "itest", "test", basic::make_document( kvp( "_id", id ) ) );
    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] byId - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end());
    assert( view.find( "result" ) != view.end());
    assert( view.find( "results" ) == view.end());

    const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", view );
    assert( dv["_id"].get_oid().value == id );
    const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
    assert( key );
    assert( *key == "value api" );
  }

  boost::asio::awaitable<void> byProperty(
      std::string_view name, std::string_view value, bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::retrieve( "itest", "test", basic::make_document( kvp( name, value ) ) );
    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
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
      assert( key == "value api" );
    }
    assert( found );
  }

  boost::asio::awaitable<void> update( bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::update( "itest", "test",
        basic::make_document(
            kvp( "key1", "value1" ),
            kvp( "date", bsoncxx::types::b_date{
                std::chrono::system_clock::now() } ),
            kvp( "_id", id ) ) );

    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
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
    assert( *key == "value api" );

    key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
    assert( key );
    assert( *key == "value1" );
  }

  boost::asio::awaitable<void> remove( bsoncxx::oid id )
  {
    namespace basic = bsoncxx::builder::basic;
    using basic::kvp;

    auto req = spt::mongoservice::api::Request::_delete( "itest", "test", basic::make_document( kvp( "_id", id ) ) );
    auto [type, opt] = co_await spt::mongoservice::api::executeAsync( std::move( req ) );
    assert( type == spt::mongoservice::api::ResultType::success );
    assert( opt );

    auto view = opt->view();
    std::cout << "[coro-client] remove - " << bsoncxx::to_json( view ) << '\n';
    assert( view.find( "error" ) == view.end() );
    assert( view.find( "success" ) != view.end() );
    assert( view.find( "history" ) != view.end() );
  }
}

boost::asio::awaitable<void> spt::client::apicrud()
{
  using namespace std::string_view_literals;
  const auto oid = bsoncxx::oid{};

  auto vhid = co_await papi::create( oid );
  std::cout << "[coro-client] vhid - " << vhid.to_string() << '\n';
  co_await papi::count();
  co_await papi::byId( oid );
  co_await papi::byProperty( "key"sv, "value api"sv, oid );
  co_await papi::update( oid );
  co_await papi::remove( oid );
  ++Status::instance().counter;
}
