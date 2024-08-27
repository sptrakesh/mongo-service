//
// Created by Rakesh on 19/07/2020.
//

#include "pool.hpp"
#include "model/configuration.hpp"
#include "../log/NanoLog.hpp"

#include <bsoncxx/builder/stream/document.hpp>

#include <future>
#include <sstream>

using spt::db::Pool;

Pool::Pool()
{
  auto uri = mongocxx::uri{ model::Configuration::instance().mongoUri };

  std::ostringstream os;
  bool first = true;
  for ( auto& host : uri.hosts() )
  {
    if ( !first ) os << ", ";
    os << host.name;
    first = false;
  }
  LOG_INFO << "Mongo host(s): " << os.str();
  LOG_INFO << "Mongo user: " << uri.username();
  if ( uri.max_pool_size() )
  {
    LOG_INFO << "Max pool size: " << *uri.max_pool_size();
  }

  pool = std::make_unique<mongocxx::pool>( uri );
  index();
}

Pool& Pool::instance()
{
  static Pool instance;
  return instance;
}

std::optional<mongocxx::pool::entry> Pool::acquire()
{
  const auto fn = [this]() { return pool->acquire(); };
  std::future<mongocxx::pool::entry> future = std::async( std::launch::async, fn );
  if ( !future.valid() )
  {
    LOG_WARN << "Error waiting for connection";
    return std::nullopt;
  }

  auto status = future.wait_for( std::chrono::seconds { 1 } );
  if ( status == std::future_status::ready ) return future.get();

  LOG_WARN << "Future timed out";
  return std::nullopt;
}

void spt::db::Pool::index()
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  const auto& config = model::Configuration::instance();
  auto cliento = acquire();

  try
  {
    if ( !cliento )
    {
      LOG_WARN << "Unable to get connection";
      return;
    }

    auto& client = *cliento;
    auto vdb = ( *client )[config.versionHistoryDatabase];
    vdb[config.versionHistoryCollection].create_index(
        document{} << "database" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "collection" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "entity._id" << 1 << finalize );

    auto mdb = ( *client )[config.metrics.database];
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error creating indices\n" << ex.what();
  }
}
