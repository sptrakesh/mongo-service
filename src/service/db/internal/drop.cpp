//
// Created by Rakesh on 05/08/2023.
//

#include "internal.h"
#include "db/pool.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "../log/NanoLog.h"
#include "../common/util/bson.h"

#include <future>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>

using std::operator""sv;

namespace spt::db::internal::pdrop
{
  void remove( std::string database, std::string collection )
  {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    LOG_INFO << "Removing all version history documents for " << database << ':' << collection;

    try
    {
      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        return;
      }

      auto& client = *cliento;
      const auto& conf = model::Configuration::instance();
      const auto res = ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].delete_many(
          document{} << "database"sv << database << "collection"sv << collection << finalize );

      if ( res )
      {
        LOG_INFO << "Deleted " << res->deleted_count() << " documents for " << database << ':' << collection;
      }
    }
    catch ( const mongocxx::bulk_write_exception& e )
    {
      LOG_WARN << "Error removing version history documents for " << database << ':' << collection << ". " << e.what();
    }
    catch ( const std::exception& e )
    {
      LOG_WARN << "Error removing version history documents for " << database << ':' << collection << ". " << e.what();
    }
  }
}

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::internal::dropCollection( const model::Document& model )
{
  using util::bsonValueIfExists;
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  const auto dbname = model.database();
  const auto collname = model.collection();
  const auto opts = model.options();

  auto cliento = Pool::instance().acquire();
  if ( !cliento )
  {
    LOG_WARN << "Connection pool exhausted";
    co_return model::poolExhausted();
  }

  auto& client = *cliento;
  ( *client )[dbname][collname].drop( opts ? writeConcern( *opts ) : mongocxx::write_concern{} );

  const auto clean = bsonValueIfExists<bool>( "clearVersionHistory", model.document() );
  if ( clean && *clean ) [[maybe_unused]] auto fut = std::async( std::launch::async, &pdrop::remove, model.database(), model.collection() );

  co_return document{} << "dropCollection" << true << finalize;
}
