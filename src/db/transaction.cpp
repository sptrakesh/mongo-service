//
// Created by Rakesh on 17/07/2021.
//

#include "pool.h"
#include "storage.h"
#include "log/NanoLog.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "util/bson.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <chrono>

namespace spt::db::internal::ptransaction
{
  struct Result
  {
    bsoncxx::builder::basic::array vhidc{};
    bsoncxx::builder::basic::array vhidd{};
    std::chrono::time_point<std::chrono::system_clock> now{ std::chrono::system_clock::now() };
    int created{ 0 };
    int deleted{ 0 };
    int updated{ 0 };
  };

  bool process( mongocxx::client_session* session, const mongocxx::pool::entry& client,
      const model::Document& doc, Result& result )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;

    const auto& conf = model::Configuration::instance();
    const auto dv = doc.document();
    const auto action = doc.action();
    const auto dbname = doc.database();
    const auto collname = doc.collection();
    const auto skip = doc.skipVersion();

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", dv );
    if ( !idopt )
    {
      LOG_WARN << "Document id not specified " << doc.json();
      session->abort_transaction();
      return false;
    }

    if ( dbname == conf.versionHistoryDatabase && collname == conf.versionHistoryCollection )
    {
      LOG_WARN << "Attempting to create in version history " << doc.json();
      session->abort_transaction();
      return false;
    }

    if ( action == "create" )
    {
      ( *client )[dbname][collname].insert_one( *session, dv );
      ++result.created;

      if ( !skip || !*skip )
      {
        auto oid = bsoncxx::oid{};
        ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one(
            *session,
            document{} <<
              "_id" << oid <<
              "database" << dbname <<
              "collection" << collname <<
              "action" << action <<
              "entity" << dv <<
              "created" << bsoncxx::types::b_date{ result.now } <<
              finalize );
        result.vhidc.append( oid );
      }
    }
    else if ( doc.action() == "update" )
    {
    }
    else if ( doc.action() == "delete" )
    {
      if ( !skip || !*skip )
      {
        auto results = ( *client )[dbname][collname].find( *session, dv );
        for ( auto&& e : results )
        {
          auto oid = bsoncxx::oid{};
          ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one(
              *session,
              document{} <<
                "_id" << oid <<
                "database" << dbname <<
                "collection" << collname <<
                "action" << action <<
                "entity" << e <<
                "created" << bsoncxx::types::b_date{ result.now } <<
                finalize );
          result.vhidd.append( oid );
        }
      }

      auto dr = ( *client )[dbname][collname].delete_many( *session, dv );
      if ( dr ) result.deleted += dr->deleted_count();
      else ++result.deleted;
    }

    return true;
  }
}

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::internal::transaction(
    const model::Document& model )
{
  using spt::util::bsonValue;
  using spt::util::bsonValueIfExists;

  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::open_document;
  using bsoncxx::builder::stream::close_document;
  using bsoncxx::builder::stream::finalize;

  LOG_DEBUG << "Executing transaction";
  const auto array = util::bsonValueIfExists<bsoncxx::array::view>( "items", model.document() );
  if ( !array )
  {
    LOG_WARN << "No items array in transaction payload";
    co_return model::missingField();
  }

  auto cliento = Pool::instance().acquire();
  if ( !cliento )
  {
    LOG_WARN << "Connection pool exhausted";
    co_return model::poolExhausted();
  }

  const auto& client = *cliento;
  auto result = ptransaction::Result{};

  const auto cb = [&]( mongocxx::client_session* session )
  {
    for ( auto&& d : *array )
    {
      const auto doc = model::Document{ d.get_document().view() };
      if ( !doc.valid() )
      {
        if ( doc.bson() )
        {
          LOG_WARN << "Skipping invalid document in transaction array " << bsoncxx::to_json( *doc.bson() );
        }
        continue;
      }

      if ( ! ptransaction::process( session, client, doc, result ) ) break;
    }
  };

  if ( !client->write_concern().is_acknowledged() )
  {
    auto w = mongocxx::write_concern{};
    w.journal( true );
    w.acknowledge_level( mongocxx::write_concern::level::k_majority );

    auto t = mongocxx::options::transaction{};
    t.write_concern( w );

    auto opts = mongocxx::options::client_session{};
    opts.default_transaction_opts( t );

    client->write_concern_deprecated( w );
  }
  auto session = client->start_session();
  try
  {
    session.with_transaction( cb );
  }
  catch ( const mongocxx::exception& e )
  {
    LOG_WARN << "Error executing transaction " << e.what();
    co_return model::transactionError();
  }

  const auto& conf = model::Configuration::instance();
  co_return document{} <<
    "created" << result.created <<
    "updated" << result.updated <<
    "deleted" << result.deleted <<
    "history" <<
      open_document <<
        "database" << conf.versionHistoryDatabase <<
        "collection" << conf.versionHistoryCollection <<
        "created" << result.vhidc.extract() <<
        "deleted" << result.vhidd.extract() <<
      close_document <<
    finalize;
}

