//
// Created by Rakesh on 22/02/2022.
//

#include "api.hpp"
#include "impl/asyncconnection.hpp"
#include "impl/connection.hpp"
#include "impl/settings.hpp"
#include "pool/pool.hpp"

#if defined __has_include
  #if __has_include("../common/magic_enum/magic_enum.hpp")
    #include "../common/magic_enum/magic_enum.hpp"
  #else
    #include <magic_enum/magic_enum.hpp>
  #endif
  #if __has_include("../common/util/defer.hpp")
    #include "../common/util/defer.hpp"
  #else
    #include <mongo-service/common/util/defer.hpp>
  #endif
#endif

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace
{
  namespace papi
  {
    using namespace spt::mongoservice;

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
      pool::Pool<api::impl::Connection> pool{ api::impl::create, api::impl::ApiSettings::instance().configuration };
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
      pool::Pool<api::impl::AsyncConnection> pool{ spt::mongoservice::api::impl::createAsyncConnection, api::impl::ApiSettings::instance().configuration };
    };
  }
}

void spt::mongoservice::api::init( std::string_view server, std::string_view port,
    std::string_view application, const pool::Configuration& poolConfiguration,
    boost::asio::io_context& ioc )
{
  auto& s = const_cast<impl::ApiSettings&>( impl::ApiSettings::instance() );
  auto lock = std::unique_lock( s.mutex );
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

auto spt::mongoservice::api::execute( bsoncxx::document::view document, std::size_t bufSize ) -> Response
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

auto spt::mongoservice::api::execute( bsoncxx::document::view document, ilp::APMRecord& apm, std::size_t bufSize ) -> Response
{
  auto& p = ilp::addProcess( apm, ilp::APMRecord::Process::Type::Function );
  DEFER( ilp::setDuration( p ) );

  auto proxy = papi::PoolHolder::instance().acquire();
  if ( !proxy )
  {
    LOG_CRIT << "Error acquiring connection from pool. APM id: " << apm.id;
    p.values.try_emplace( "error", "Pool exhausted" );
    return { ResultType::poolFailure, std::nullopt };
  }

  auto& connection = proxy.value().operator*();
  auto idx = apm.processes.size();
  auto opt = connection.execute( document, apm, bufSize );
  ilp::addCurrentFunction( *apm.processes[idx] );
  if ( !opt )
  {
    LOG_WARN << "Error executing command " << bsoncxx::to_json( document ) << ". APM id: " << apm.id;
    p.values.try_emplace( "error", "Command failed" );
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
  using std::operator ""sv;

  auto action = ( model::request::Action::_delete == req.action ) ? "delete"sv : magic_enum::enum_name( req.action );
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
  if ( req.skipMetric ) query << "skipMetric" << req.skipMetric;

  const auto q = query << finalize;
  return execute( q.view(), bufSize );
}

auto spt::mongoservice::api::execute( const Request& req, ilp::APMRecord& apm, std::size_t bufSize ) -> Response
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::open_document;
  using bsoncxx::builder::stream::close_document;
  using bsoncxx::builder::stream::finalize;
  using std::operator ""sv;

  auto& p = ilp::addProcess( apm, ilp::APMRecord::Process::Type::Function );
  DEFER( ilp::setDuration( p ) );

  auto action = ( model::request::Action::_delete == req.action ) ? "delete"sv : magic_enum::enum_name( req.action );
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
  if ( req.skipMetric ) query << "skipMetric" << req.skipMetric;

  const auto q = query << finalize;
  auto idx = apm.processes.size();
  auto resp = execute( q.view(), apm, bufSize );
  ilp::addCurrentFunction( *apm.processes[idx] );
  return resp;
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
  using std::operator ""sv;

  auto action = ( model::request::Action::_delete == req.action ) ? "delete"sv : magic_enum::enum_name( req.action );
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
  if ( req.skipMetric ) query << "skipMetric" << req.skipMetric;

  auto q = query << finalize;
  co_return co_await executeAsync( q.view() );
}
