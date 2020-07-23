//
// Created by Rakesh on 20/07/2020.
//

#include "storage.h"
#include "pool.h"
#include "log/NanoLog.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "util/bson.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <chrono>

namespace spt::db::pstorage
{
  bsoncxx::document::view_or_value history( bsoncxx::document::view view, mongocxx::pool::entry& client,
      std::optional<bsoncxx::document::view> metadata = std::nullopt )
  {
    using spt::util::bsonValue;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto& conf = model::Configuration::instance();
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto id = bsonValue<bsoncxx::oid>( "_id", doc );

    auto oid = bsoncxx::oid{};
    auto d = document{};
    d << "_id" << oid <<
      "database" << dbname <<
      "collection" << collname <<
      "action" << bsonValue<std::string>( "action", view ) <<
      "entity" << doc <<
      "created" << bsoncxx::types::b_date{ std::chrono::system_clock::now() };
    if ( metadata ) d << "metadata" << *metadata;

    const auto vr = (*client)[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one(
        d << finalize );
    LOG_INFO << "Created version for " << dbname << ':' << collname << ':' <<
             id.to_string() << " with id: " << oid.to_string();
    return document{} << "_id" << oid <<
      "database" << conf.versionHistoryDatabase <<
      "collection" << conf.versionHistoryCollection <<
      "entity" << id << finalize;
  }

  mongocxx::write_concern writeConcern( bsoncxx::document::view view )
  {
    using spt::util::bsonValueIfExists;

    auto w = mongocxx::write_concern{};

    auto journal = bsonValueIfExists<bool>( "journal", view );
    if ( journal ) w.journal( *journal );

    auto nodes = bsonValueIfExists<int32_t>( "nodes", view );
    if ( nodes ) w.nodes( *nodes );

    auto level = bsonValueIfExists<int32_t>( "acknowledgeLevel", view );
    if ( level ) w.acknowledge_level( static_cast<mongocxx::write_concern::level>( *level ) );
    else w.acknowledge_level( mongocxx::write_concern::level::k_majority );

    auto majority = bsonValueIfExists<std::chrono::milliseconds>( "majority", view );
    if ( majority ) w.majority( *majority );

    auto tag = bsonValueIfExists<std::string>( "tag", view );
    if ( tag ) w.tag( *tag );

    auto timeout = bsonValueIfExists<std::chrono::milliseconds>( "timeout", view );
    if ( timeout ) w.timeout( *timeout );

    return w;
  }

  bsoncxx::document::view_or_value create( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", view );

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( !idopt ) return model::missingId();

    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::insert{};
    if ( options )
    {
      auto ordered = bsonValueIfExists<bool>( "ordered", *options );
      if ( ordered ) opts.ordered( *ordered );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ) );
    }

    auto client = Pool::instance().acquire();
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );
    const auto result = (*client)[dbname][collname].insert_one( doc, opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
        return history( view, client, metadata );
      }
      else
      {
        LOG_WARN << "Unable to create document " << dbname << ':' << collname << ':' << idopt->to_string();
      }
    }
    else
    {
      LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
      return history( view, client, metadata );
    }

    return model::insertError();
  }

  mongocxx::options::find findOpts( bsoncxx::document::view view )
  {
    using spt::util::bsonValueIfExists;

    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::find{};

    if ( options )
    {
      const auto partial = bsonValueIfExists<bool>( "partialResults", *options );
      if ( partial ) opts.allow_partial_results( *partial );

      const auto size = bsonValueIfExists<int32_t>( "batchSize", *options );
      if ( size ) opts.batch_size( *size );

      const auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( co ) opts.collation( *co );

      const auto com = bsonValueIfExists<std::string>( "comment", *options );
      if ( com ) opts.comment( *com );

      const auto hint = bsonValueIfExists<bsoncxx::document::view>( "hint", *options );
      if ( hint ) opts.hint( { *hint } );

      const auto limit = bsonValueIfExists<int64_t>( "limit", *options );
      if ( limit ) opts.limit( *limit );

      const auto max = bsonValueIfExists<bsoncxx::document::view>( "max", *options );
      if ( max ) opts.max( *max );

      const auto maxTime = bsonValueIfExists<std::chrono::milliseconds>( "maxTime", *options );
      if ( maxTime ) opts.max_time( *maxTime );

      const auto min = bsonValueIfExists<bsoncxx::document::view>( "min", *options );
      if ( min ) opts.min( *min );

      const auto projection = bsonValueIfExists<bsoncxx::document::view>( "projection", *options );
      if ( projection ) opts.projection( *projection );

      const auto rp = bsonValueIfExists<int32_t>( "readPreference", *options );
      if ( rp )
      {
        auto p = mongocxx::read_preference{};
        p.mode( static_cast<mongocxx::read_preference::read_mode>( *rp ) );
        opts.read_preference( p );
      }

      const auto rk = bsonValueIfExists<bool>( "returnKey", *options );
      if ( rk ) opts.return_key( *rk );

      const auto ri = bsonValueIfExists<bool>( "showRecordId", *options );
      if ( ri ) opts.show_record_id( *ri );

      const auto skip = bsonValueIfExists<int64_t>( "skip", *options );
      if ( skip ) opts.skip( *skip );

      const auto sort = bsonValueIfExists<bsoncxx::document::view>( "sort", *options );
      if ( sort ) opts.sort( *sort );
    }

    return opts;
  }

  bsoncxx::document::view_or_value retrieveOne( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto id = bsonValue<bsoncxx::oid>( "_id", doc );
    const auto opts = findOpts( view );

    auto client = Pool::instance().acquire();
    const auto res = (*client)[dbname][collname].find_one(
        document{} << "_id" << id << finalize, opts );
    if ( res ) return document{} << "result" << res->view() << finalize;

    LOG_WARN << "Document not found: " << dbname << ':' << collname << ':' << id.to_string();
    return model::notFound();
  }

  bsoncxx::document::view_or_value retrieve( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::basic::kvp;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( idopt ) return retrieveOne( view );

    const auto opts = findOpts( view );
    auto client = Pool::instance().acquire();
    auto cursor = (*client)[dbname][collname].find( doc, opts );

    auto array = bsoncxx::builder::basic::array{};
    for ( auto d : cursor ) array.append( d );
    return bsoncxx::builder::basic::make_document( kvp("results", array) );
  }

  bsoncxx::document::view_or_value remove( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;

    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", view );

    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::delete_options{};
    if ( options )
    {
      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ) );

      auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( co ) opts.collation( *co );
    }

    const auto docs = retrieve( view );
    const auto results = bsonValueIfExists<bsoncxx::array::view>( "results", docs );
    auto client = Pool::instance().acquire();

    auto success = bsoncxx::builder::basic::array{};
    auto vh = bsoncxx::builder::basic::array{};

    if ( results )
    {
      for ( auto item : *results )
      {
        const auto d = item.get_document().view();
        const auto oid = bsonValue<bsoncxx::oid>( "_id", d );
        (*client)[dbname][collname].delete_one(
            document{} << "_id" << oid << finalize, opts );
        LOG_INFO << "Deleted document " << dbname << ':' << collname << ':' << oid.to_string();
        success.append( oid );

        auto vhd =  history( document{} << "action" << "delete" <<
          "database" << dbname << "collection" << collname <<
          "document" << item.get_value().get_document().value << finalize, client, metadata );
        vh.append( vhd );
      }

      return document{} << "success" << success << "history" << vh << finalize;
    }

    const auto result = bsonValueIfExists<bsoncxx::document::view>( "result", docs );
    if ( result )
    {
      const auto oid = bsonValue<bsoncxx::oid>( "_id", *result );
      (*client)[dbname][collname].delete_one(
          document{} << "_id" << oid << finalize, opts );
      LOG_INFO << "Deleted document " << dbname << ':' << collname << ':' << oid.to_string();
      success.append( oid );

      auto vhd =  history( document{} << "action" << "delete" <<
        "database" << dbname << "collection" << collname <<
        "document" << *result << finalize, client, metadata );
      return document{} << "success" << success << "history" << vh << finalize;
    }

    return model::notFound();
  }
}

bsoncxx::document::view_or_value spt::db::process( bsoncxx::document::view view )
{
  using spt::util::bsonValue;

  try
  {
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
      return pstorage::retrieve( view );
    }
    else if ( action == "delete" )
    {
      return pstorage::remove( view );
    }

    return bsoncxx::document::value{ model::invalidAction() };
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing database action " << ex.what();
    LOG_INFO << bsoncxx::to_json( view );
  }

  return model::unexpectedError();
}
