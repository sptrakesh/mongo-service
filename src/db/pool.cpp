//
// Created by Rakesh on 19/07/2020.
//

#include "pool.h"
#include "log/NanoLog.h"
#include "model/configuration.h"

#include <bsoncxx/builder/stream/document.hpp>

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

  pool = std::make_unique<mongocxx::pool>( uri );
  index();
}

Pool& Pool::instance()
{
  static Pool instance;
  return instance;
}

mongocxx::pool::entry Pool::acquire()
{
  return pool->acquire();
}

void spt::db::Pool::index()
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  auto& config = model::Configuration::instance();
  auto client = acquire();

  try
  {
    auto vdb = ( *client )[config.versionHistoryDatabase];
    vdb[config.versionHistoryCollection].create_index(
        document{} << "database" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "collection" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "action" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "entity._id" << 1 << finalize );
    vdb[config.versionHistoryCollection].create_index(
        document{} << "created" << 1 << finalize );

    auto mdb = ( *client )[config.metricsDatabase];
    mdb[config.metricsCollection].create_index(
        document{} << "action" << 1 << finalize );
    mdb[config.metricsCollection].create_index(
        document{} << "correlationId" << 1 << finalize );
    mdb[config.metricsCollection].create_index(
        document{} << "application" << 1 << finalize );
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error creating indices\n" << ex.what();
  }
}
