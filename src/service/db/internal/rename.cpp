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
#include <mongocxx/exception/logic_error.hpp>

using std::operator""sv;

namespace spt::db::internal::prename
{
  void update( std::string database, std::string collection, std::string target )
  {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;

    LOG_INFO << "Updating version history documents associated with " <<
      database << ':' << collection << " to " <<
      database << ':' << target;

    try
    {
      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        return;
      }

      const auto& client = *cliento;
      const auto& conf = model::Configuration::instance();
      const auto res = ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].update_many(
          document{} << "database"sv << database << "collection"sv << collection << finalize,
          document{} <<
            "$set" << open_document << "collection"sv << target << close_document <<
            finalize
      );

      if ( res )
      {
        LOG_INFO << "Update query matched " << res->matched_count() <<
          " and updated " << res->modified_count() << " documents from " <<
          database << ':' << collection << " to " <<
          database << ':' << target;
      }
    }
    catch ( const mongocxx::logic_error& ex )
    {
      LOG_WARN << "Error renaming version history documents collection from " <<
          database << ':' << collection << " to " <<
          database << ':' << target << ". " << ex.what();
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error renaming version history documents collection from " <<
        database << ':' << collection << " to " <<
        database << ':' << target << ". " << ex.what();
    }
  }
}

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::internal::renameCollection(
    const spt::model::Document& model )
{
  using spt::util::bsonValue;
  using spt::util::bsonValueIfExists;

  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  if ( model.database().empty() ) co_return model::missingField();
  if ( model.collection().empty() ) co_return model::missingField();
  const auto target = bsonValueIfExists<std::string>( "target"sv, model.document() );
  if ( !target )
  {
    LOG_WARN << "No target collection name specified in document. " << model.json();
    co_return model::missingField();
  }

  LOG_INFO << "Renaming collection " << model.database() << ':' << model.collection() << " to " <<
    model.database() << ':' << *target;

  const auto opts = model.options();
  auto cliento = Pool::instance().acquire();
  if ( !cliento )
  {
    LOG_WARN << "Connection pool exhausted";
    co_return model::poolExhausted();
  }

  const auto& client = *cliento;
  if ( ( *client )[model.database()].has_collection( *target ) )
  {
    LOG_WARN << "Target database " << *target << " exists in database " << model.database();
    co_return model::withMessage( "Target exists in database"sv );
  }

  ( *client )[model.database()][model.collection()].rename( *target, false, opts ? writeConcern( *opts ) : mongocxx::write_concern{} );

  [[maybe_unused]] auto fut = std::async( std::launch::async, &prename::update, model.database(), model.collection(), *target );

  co_return document{} << "database"sv << model.database() << "collection"sv << *target << finalize;
}
