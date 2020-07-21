//
// Created by Rakesh on 20/07/2020.
//

#include "storage.h"
#include "pool.h"
#include "log/NanoLog.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "util/bson.h"

#include <bsoncxx/builder/stream/document.hpp>

#include <chrono>

namespace spt::db::pstorage
{
  void history( bsoncxx::document::view view, mongocxx::pool::entry& client )
  {
    using spt::util::bsonValue;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto& conf = model::Configuration::instance();
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto id = bsonValue<bsoncxx::oid>( "_id", doc );

    const auto vr = (*client)[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one(
        document{} <<
          "database" << dbname <<
          "collection" << collname <<
          "action" << bsonValue<std::string>( "action", view ) <<
          "entity" << doc <<
          "created" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
        finalize );
    if ( vr )
    {
      LOG_INFO << "Created version for " << dbname << ':' << collname << id.to_string() <<
               " with id: " << vr->inserted_id().get_oid().value.to_string();
    }
  }

  bsoncxx::document::view_or_value create( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( !idopt ) return model::missingId();

    auto client = Pool::instance().acquire();

    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::insert{};
    if ( options )
    {
      auto ordered = bsonValueIfExists<bool>( "ordered", *options );
      if ( ordered ) opts.ordered( *ordered );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      auto w = mongocxx::write_concern{};

      auto journal = bsonValueIfExists<bool>( "journal", *wc );
      if ( journal ) w.journal( *journal );

      auto nodes = bsonValueIfExists<int32_t>( "nodes", *wc );
      if ( nodes ) w.nodes( *nodes );

      auto level = bsonValueIfExists<int32_t>( "acknowledgeLevel", *wc );
      if ( level ) w.acknowledge_level( static_cast<mongocxx::write_concern::level>( *level ) );

      auto majority = bsonValueIfExists<std::chrono::milliseconds>( "majority", *wc );
      if ( majority ) w.majority( *majority );

      auto tag = bsonValueIfExists<std::string>( "tag", *wc );
      if ( tag ) w.tag( *tag );

      auto timeout = bsonValueIfExists<std::chrono::milliseconds>( "timeout", *wc );
      if ( timeout ) w.timeout( *timeout );
    }

    const auto result = (*client)[dbname][collname].insert_one( doc, opts );
    if ( result )
    {
      LOG_INFO << "Created " << dbname << ':' << collname << idopt->to_string() << " document";
      history( view, client );
    }
    else
    {
      return model::insertError();
    }

    return doc;
  }
}

bsoncxx::document::view_or_value spt::db::process( bsoncxx::document::view view )
{
  using spt::util::bsonValue;

  const auto action = bsonValue<std::string>( "action", view );
  if ( action == "create" )
  {
    return pstorage::create( view );
  }
  else if ( action == "update" )
  {
  }
  else if ( action == "retrieve" )
  {
  }
  else if ( action == "delete" )
  {
  }

  return bsoncxx::document::value{ model::invalidAction() };
}
