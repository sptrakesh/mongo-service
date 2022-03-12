//
// Created by Rakesh on 22/02/2022.
//

#include "api.h"
#include "impl/asyncconnection.h"
#include "impl/connection.h"
#include "impl/settings.h"
#include "pool/pool.h"
#if __has_include("../common/util/magic_enum.hpp")
#include "../common/util/magic_enum.hpp"
#else
#include <util/magic_enum.hpp>
#endif

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::papi
{
  struct PoolHolder
  {
    static PoolHolder& instance()
    {
      static PoolHolder p;
      return p;
    }

    ~PoolHolder() = default;
    PoolHolder(const PoolHolder&) = delete;
    PoolHolder& operator=(const PoolHolder&) = delete;

    auto acquire() { return pool.acquire(); }

  private:
    PoolHolder() = default;
    pool::Pool<impl::Connection> pool{ spt::mongoservice::api::impl::create, impl::ApiSettings::instance().configuration };
  };

  struct AsyncPoolHolder
  {
    static AsyncPoolHolder& instance()
    {
      static AsyncPoolHolder p;
      return p;
    }

    ~AsyncPoolHolder() = default;
    AsyncPoolHolder(const AsyncPoolHolder&) = delete;
    AsyncPoolHolder& operator=(const AsyncPoolHolder&) = delete;

    auto acquire() { return pool.acquire(); }

  private:
    AsyncPoolHolder() = default;
    pool::Pool<impl::AsyncConnection> pool{ spt::mongoservice::api::impl::createAsyncConnection, impl::ApiSettings::instance().configuration };
  };
}

void spt::mongoservice::api::init( std::string_view server, std::string_view port,
    std::string_view application, const pool::Configuration& poolConfiguration,
    boost::asio::io_context& ioc )
{
  auto& s = const_cast<impl::ApiSettings&>( impl::ApiSettings::instance() );
  auto lock = std::unique_lock<std::mutex>( s.mutex );
  if ( s.ioc == nullptr )
  {
    s.server.append( server.data(), server.size() );
    s.port.append( port.data(), port.size() );
    s.application.append( application.data(), application.size() );
    s.configuration = poolConfiguration;
    s.ioc = &ioc;
  }
  else
  {
    LOG_CRIT << "API init called multiple times.";
  }
}

auto spt::mongoservice::api::execute(
    bsoncxx::document::view document, std::size_t bufSize ) -> Response
{
  auto proxy = papi::PoolHolder::instance().acquire();
  if ( !proxy )
  {
    LOG_CRIT << "Error acquiring connection from pool";
    return { ResultType::poolFailure, std::nullopt };
  }

  auto& connection = proxy.value().operator*();
  auto opt = connection.execute( document, bufSize );
  if ( !opt )
  {
    LOG_WARN << "Error executing command " << bsoncxx::to_json( document );
    connection.setValid( false );
    return { ResultType::commandFailure, std::nullopt };
  }

  return { ResultType::success, std::move( opt ) };
}

auto spt::mongoservice::api::execute( const Request& req, std::size_t bufSize ) -> Response
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::open_document;
  using bsoncxx::builder::stream::close_document;
  using bsoncxx::builder::stream::finalize;
  using namespace std::string_view_literals;

  auto action = ( Request::Action::_delete == req.action ) ? "delete"sv : magic_enum::enum_name( req.action );
  auto query = document{};
  query <<
    "action" << action <<
    "database" << req.database <<
    "collection" << req.collection <<
    "document" << req.document <<
    "application" << impl::ApiSettings::instance().application;

  if ( req.options ) query << "options" << *req.options;
  if ( req.metadata ) query << "metadata" << *req.metadata;
  if ( req.correlationId ) query << "correlationId" << *req.correlationId;
  if ( req.skipVersion ) query << "skipVersion" << req.skipVersion;

  const auto q = query << finalize;
  return execute( q.view(), bufSize );
}

auto spt::mongoservice::api::executeAsync( bsoncxx::document::view document ) -> AsyncResponse
{
  auto proxy = papi::AsyncPoolHolder::instance().acquire();
  if ( !proxy )
  {
    LOG_CRIT << "Error acquiring connection from pool";
    co_return Response{ ResultType::poolFailure, std::nullopt };
  }

  auto& connection = proxy.value().operator*();
  auto opt = co_await connection.execute( document );
  if ( !opt )
  {
    LOG_WARN << "Error executing command " << bsoncxx::to_json( document );
    connection.setValid( false );
    co_return Response{ ResultType::commandFailure, std::nullopt };
  }

  co_return Response{ ResultType::success, std::move( opt ) };
}

auto spt::mongoservice::api::executeAsync( Request req ) -> AsyncResponse
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::open_document;
  using bsoncxx::builder::stream::close_document;
  using bsoncxx::builder::stream::finalize;

  auto action = magic_enum::enum_name( req.action );
  if ( Request::Action::_delete == req.action ) action = std::string{ "delete" };
  auto query = document{};
  query <<
    "action" << action <<
    "database" << req.database <<
    "collection" << req.collection <<
    "document" << req.document <<
    "application" << impl::ApiSettings::instance().application;

  if ( req.options ) query << "options" << *req.options;
  if ( req.metadata ) query << "metadata" << *req.metadata;
  if ( req.correlationId ) query << "correlationId" << *req.correlationId;
  if ( req.skipVersion ) query << "skipVersion" << req.skipVersion;

  auto q = query << finalize;
  co_return co_await executeAsync( q.view() );
}
