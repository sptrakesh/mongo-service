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
    if ( client->write_concern().is_acknowledged() )
    {
      if ( vr )
      {
        LOG_INFO << "Created version for " << dbname << ':' << collname << ':' <<
                 id.to_string() << " with id: " << oid.to_string();
        return document{} << "_id" << oid <<
          "database" << conf.versionHistoryDatabase <<
          "collection" << conf.versionHistoryCollection <<
          "entity" << id << finalize;
      }
      else
      {
        LOG_WARN << "Unable to create version for " << dbname << ':' << collname << ':' <<
                 id.to_string();
      }
    }
    else
    {
      LOG_INFO << "Created version for " << dbname << ':' << collname << ':' <<
               id.to_string() << " with id: " << oid.to_string();
      return document{} << "_id" << oid <<
        "database" << conf.versionHistoryDatabase <<
        "collection" << conf.versionHistoryCollection <<
        "entity" << id << finalize;
    }

    return model::createVersionFailed();
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
      auto validate = bsonValueIfExists<bool>( "bypassValidation", *options );
      if ( validate ) opts.bypass_document_validation( *validate );

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

  bsoncxx::document::value updateDoc( bsoncxx::document::view doc )
  {
    using spt::util::bsonValue;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;

    auto d = document{};
    d << "$set" << open_document;

    for ( auto e : doc )
    {
      if ( e.key() == "_id" ) continue;

      switch ( e.type() )
      {
      case bsoncxx::type::k_array:
        d << e.key() << e.get_array();
        break;
      case bsoncxx::type::k_bool:
        d << e.key() << e.get_bool();
        break;
      case bsoncxx::type::k_date:
        d << e.key() << e.get_date();
        break;
      case bsoncxx::type::k_decimal128:
        d << e.key() << e.get_decimal128();
        break;
      case bsoncxx::type::k_document:
        d << e.key() << e.get_document();
        break;
      case bsoncxx::type::k_double:
        d << e.key() << e.get_double();
        break;
      case bsoncxx::type::k_int32:
        d << e.key() << e.get_int32();
        break;
      case bsoncxx::type::k_int64:
        d << e.key() << e.get_int64();
        break;
      case bsoncxx::type::k_oid:
        d << e.key() << e.get_oid();
        break;
      case bsoncxx::type::k_null:
        d << e.key() << e.get_null();
        break;
      case bsoncxx::type::k_timestamp:
        d << e.key() << e.get_timestamp();
        break;
      case bsoncxx::type::k_utf8:
        d << e.key() << bsonValue<std::string>( e.key(), doc );
        break;
      default:
        LOG_WARN << "Un-mapped bson type: " << bsoncxx::to_string( e.type() );
        d << e.key() << bsoncxx::types::b_null{};
      }
    }

    d << close_document;
    return d << finalize;
  }

  mongocxx::options::update updateOptions( bsoncxx::document::view view )
  {
    using spt::util::bsonValueIfExists;

    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::update{};

    if ( options )
    {
      auto validate = bsonValueIfExists<bool>( "bypassValidation", *options );
      if ( validate ) opts.bypass_document_validation( *validate );

      auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( col ) opts.collation( *col );

      auto upsert = bsonValueIfExists<bool>( "upsert", *options );
      if ( upsert ) opts.upsert( *upsert );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ) );

      auto af = bsonValueIfExists<bsoncxx::array::view>( "arrayFilters", *options );
      if ( af ) opts.array_filters( *af );
    }

    return opts;
  }

  bsoncxx::document::view_or_value updateOne( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", view );
    const auto oid = bsonValue<bsoncxx::oid>( "_id", doc );

    auto opts = updateOptions( view );
    auto client = Pool::instance().acquire();
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto vhd = [&dbname, &collname, &client, &metadata, &oid]() -> bsoncxx::document::view_or_value
    {
      const auto updated = (*client)[dbname][collname].find_one(
          document{} << "_id" << oid << finalize );
      if ( !updated ) return model::notFound();
      if ( bsonValueIfExists<std::string>( "error", *updated ) ) return { updated.value() };

      auto vhd =  history( document{} << "action" << "update" <<
        "database" << dbname << "collection" << collname <<
        "document" << updated->view() << finalize, client, metadata );
      if ( bsonValueIfExists<std::string>( "error", vhd ) ) return vhd;

      return bsoncxx::document::view_or_value {
        document{} << "document" << updated->view() << "history" << vhd << finalize };
    };

    const auto result = (*client)[dbname][collname].update_one(
        document{} << "_id" << oid << finalize, updateDoc( doc ), opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
        return vhd();
      }
      else
      {
        LOG_WARN << "Unable to update document " << dbname << ':' << collname << ':' << oid.to_string();
      }
    }
    else
    {
      LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
      return vhd();
    }

    return model::updateError();
  }

  bsoncxx::document::view_or_value replaceOne( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", view );
    const auto filter = bsonValue<bsoncxx::document::view>( "filter", doc );

    auto client = Pool::instance().acquire();
    const auto options = bsonValueIfExists<bsoncxx::document::view>( "options", view );
    auto opts = mongocxx::options::replace{};

    if ( options )
    {
      auto validate = bsonValueIfExists<bool>( "bypassValidation", *options );
      if ( validate ) opts.bypass_document_validation( *validate );

      auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( col ) opts.collation( *col );

      auto upsert = bsonValueIfExists<bool>( "upsert", *options );
      if ( upsert ) opts.upsert( *upsert );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ) );
    }

    const auto vhd = [&dbname, &collname, &client, &metadata, &filter]() -> bsoncxx::document::view_or_value
    {
      const auto updated = (*client)[dbname][collname].find_one( filter );
      if ( !updated ) return model::notFound();

      auto vhd =  history( document{} << "action" << "replace" <<
       "database" << dbname << "collection" << collname <<
       "document" << updated->view() << finalize, client, metadata );
      if ( bsonValueIfExists<std::string>( "error", vhd ) ) return vhd;

      return document{} << "document" << updated->view() << "history" << vhd << finalize;
    };

    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );
    const auto result = (*client)[dbname][collname].replace_one( filter,
        bsonValue<bsoncxx::document::view>( "replace", doc ), opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Updated document in " << dbname << ':' << collname <<
          " with filter " << bsoncxx::to_json( filter );
        return vhd();
      }
      else
      {
        LOG_INFO << "Unable to update document in " << dbname << ':' << collname <<
          " with filter " << bsoncxx::to_json( filter );
      }
    }
    else
    {
      LOG_INFO << "Updated document in " << dbname << ':' << collname <<
        " with filter " << bsoncxx::to_json( filter );
      return vhd();
    }

    return model::updateError();
  }

  bsoncxx::document::view_or_value update( bsoncxx::document::view view )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = bsonValue<bsoncxx::document::view>( "document", view );
    const auto dbname = bsonValue<std::string>( "database", view );
    const auto collname = bsonValue<std::string>( "collection", view );
    const auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", view );

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( idopt ) return updateOne( view );

    const auto filter = bsonValueIfExists<bsoncxx::document::view>( "filter", view );
    if ( !filter ) return model::invalidAUpdate();

    const auto replace = bsonValueIfExists<bsoncxx::document::view>( "replace", view );
    if ( replace ) return replaceOne( view );

    const auto update = bsonValueIfExists<bsoncxx::document::view>( "update", view );
    if ( !update ) return model::invalidAUpdate();

    auto client = Pool::instance().acquire();
    auto opts = updateOptions( view );
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto result = (*client)[dbname][collname].update_many( *filter, updateDoc( *update ), opts );

    const auto vhd = [&dbname, &collname, &client, &filter, &metadata]() -> bsoncxx::document::view_or_value
    {
      auto success = bsoncxx::builder::basic::array{};
      auto fail = bsoncxx::builder::basic::array{};
      auto vh = bsoncxx::builder::basic::array{};
      auto results = (*client)[dbname][collname].find( *filter );

      auto docs = bsoncxx::builder::basic::array{};
      for ( auto d : results ) docs.append( d );

      for ( auto d : docs.view() )
      {
        auto vhd =  history( document{} << "action" << "update" <<
          "database" << dbname << "collection" << collname <<
          "document" << d.get_document().view() << finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", vhd ) ) fail.append( d["_id"].get_oid() );
        else
        {
          success.append( d["_id"].get_oid() );
          vh.append( vhd );
        }
      }

      return document{} << "success" << success << "failure" << fail << "history" << vh << finalize;
    };

    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
        return vhd();
      }
      else
      {
        LOG_WARN << "Unable to create document " << dbname << ':' << collname << ':' << idopt->to_string();
      }
    }
    else
    {
      LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
      return vhd();
    }

    return model::updateError();
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
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    auto success = bsoncxx::builder::basic::array{};
    auto fail = bsoncxx::builder::basic::array{};
    auto vh = bsoncxx::builder::basic::array{};

    const auto rm = [&client, &dbname, &collname, &metadata, &opts, &success, &fail, &vh]( auto d )
    {
      const auto vhd = [&client, &dbname, &collname, &metadata, &vh]( auto d )
      {
        auto vhd =  history( document{} << "action" << "delete" <<
          "database" << dbname << "collection" << collname <<
          "document" << d << finalize, client, metadata );
        vh.append( vhd );
      };

      const auto oid = bsonValue<bsoncxx::oid>( "_id", d );
      const auto res = (*client)[dbname][collname].delete_one(
          document{} << "_id" << oid << finalize, opts );

      if ( opts.write_concern()->is_acknowledged() )
      {
        if ( res )
        {
          LOG_INFO << "Deleted document " << dbname << ':' << collname << ':' << oid.to_string();
          success.append( oid );
          vhd( d );
        }
        else
        {
          LOG_WARN << "Unable to delete document " << dbname << ':' << collname << ':' << oid.to_string();
          fail.append( oid );
        }
      }
      else
      {
        LOG_INFO << "Deleted document " << dbname << ':' << collname << ':' << oid.to_string();
        success.append( oid );
        vhd( d );
      }
    };

    if ( results )
    {
      for ( auto item : *results )
      {
        const auto d = item.get_document().view();
        rm( d );
      }

      return document{} << "success" << success << "failure" << fail << "history" << vh << finalize;
    }

    const auto result = bsonValueIfExists<bsoncxx::document::view>( "result", docs );
    if ( result )
    {
      rm( *result );
      return document{} << "success" << success << "failure" << fail << "history" << vh << finalize;
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
      return pstorage::update( view );
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
