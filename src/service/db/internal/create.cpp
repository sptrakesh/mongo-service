//
// Created by Rakesh on 21/11/2024.
//

#include "internal.hpp"
#include "db/pool.hpp"
#include "model/configuration.hpp"
#include "model/errors.hpp"
#include "../log/NanoLog.hpp"
#include "../common/util/bson.hpp"

#include <future>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/write_concern.hpp>

using std::operator""sv;

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::internal::createCollection( const model::Document& model )
{
  using util::bsonValue;
  using util::bsonValueIfExists;

  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  if ( model.database().empty() ) co_return model::missingField();
  if ( model.collection().empty() ) co_return model::missingField();

  LOG_INFO << "Creating collection " << model.database() << ':' << model.collection();

  mongocxx::write_concern wc{};
  if ( const auto opts = model.options(); opts ) wc = writeConcern( *opts );

  auto cliento = Pool::instance().acquire();
  if ( !cliento )
  {
    LOG_WARN << "Connection pool exhausted";
    co_return model::poolExhausted();
  }

  const auto& client = *cliento;
  if ( ( *client )[model.database()].has_collection( model.collection() ) )
  {
    LOG_WARN << "A collection " << model.collection() << " exists in database " << model.database();
    co_return model::withMessage( "Collection exists in database"sv );
  }

  ( *client )[model.database()].create_collection( model.collection(), model.document(), wc );
  co_return document{} << "database"sv << model.database() << "collection"sv << model.collection() << finalize;
}