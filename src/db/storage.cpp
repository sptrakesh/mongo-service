//
// Created by Rakesh on 20/07/2020.
//

#include "storage.h"
#include "pool.h"
#include "log/NanoLog.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "model/metric.h"
#include "util/bson.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <chrono>
#include <sstream>

namespace spt::db::pstorage
{
  bsoncxx::document::view_or_value history( bsoncxx::document::view view,
      mongocxx::pool::entry& client,
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

    const auto vr = ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one(
        d << finalize );
    if ( client->write_concern().is_acknowledged())
    {
      if ( vr )
      {
        LOG_INFO
            << "Created version for " << dbname << ':' << collname << ':' <<
            id.to_string() << " with id: " << oid.to_string();
        return document{} << "_id" << oid <<
          "database" << conf.versionHistoryDatabase <<
          "collection" << conf.versionHistoryCollection <<
          "entity" << id << finalize;
      }
      else
      {
        LOG_WARN
          << "Unable to create version for " << dbname << ':' << collname
          << ':' <<
          id.to_string();
      }
    }
    else
    {
      LOG_INFO
          << "Created version for " << dbname << ':' << collname << ':' <<
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
    if ( level )
      w.acknowledge_level(
          static_cast<mongocxx::write_concern::level>( *level ));
    else w.acknowledge_level( mongocxx::write_concern::level::k_majority );

    auto majority = bsonValueIfExists<std::chrono::milliseconds>( "majority",
        view );
    if ( majority ) w.majority( *majority );

    auto tag = bsonValueIfExists<std::string>( "tag", view );
    if ( tag ) w.tag( *tag );

    auto timeout = bsonValueIfExists<std::chrono::milliseconds>( "timeout",
        view );
    if ( timeout ) w.timeout( *timeout );

    return w;
  }

  bsoncxx::document::view_or_value index( const model::Document& model )
  {
    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto options = model.options();

    auto client = Pool::instance().acquire();
    return options ?
        (*client)[dbname][collname].create_index( doc, *options ) :
        (*client)[dbname][collname].create_index( doc );
  }

  bsoncxx::document::view_or_value dropIndex( const model::Document& model )
  {
    using util::bsonValueIfExists;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto options = model.options();

    auto cmd = document{};
    cmd << "dropIndexes" << model.collection();

    const auto name = bsonValueIfExists<std::string>( "name", doc );
    if ( name ) cmd << "index" << *name;

    const auto spec = bsonValueIfExists<bsoncxx::document::view>( "specification", doc );
    if ( spec ) cmd << "index" << *spec;

    const auto names = bsonValueIfExists<bsoncxx::array::view>( "names", doc );
    if ( names ) cmd << "index" << *names;

    auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
    if ( wc ) cmd << "writeConcern" << writeConcern( *wc ).to_document();

    auto comment = bsonValueIfExists<std::string>( "comment", *options );
    if ( comment ) cmd << "comment" << *comment;

    auto client = Pool::instance().acquire();
    return (*client)[dbname].run_command( cmd << finalize );
  }

  bsoncxx::document::view_or_value count( const model::Document& model )
  {
    using util::bsonValueIfExists;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto opts = model.options();

    auto options = mongocxx::options::count{};
    if ( opts )
    {
      const auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *opts );
      if ( col ) options.collation( *col );

      const auto hint = bsonValueIfExists<bsoncxx::document::view>( "hint", *opts );
      if ( hint ) options.hint( mongocxx::hint{ *hint } );

      const auto limit = bsonValueIfExists<int64_t>( "limit", *opts );
      if ( limit ) options.limit( *limit );

      const auto time = bsonValueIfExists<std::chrono::milliseconds>( "maxTime", *opts );
      if ( time ) options.max_time( *time );

      const auto skip = bsonValueIfExists<int64_t>( "skip", *opts );
      if ( limit ) options.skip( *skip );
    }

    auto client = Pool::instance().acquire();
    const auto count = (*client)[dbname][collname].count_documents( model.document(), options );
    return document{} << "count" << count << finalize;
  }

  mongocxx::options::find findOpts( const model::Document& model )
  {
    using spt::util::bsonValueIfExists;

    const auto options = model.options();
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

  bsoncxx::document::view_or_value retrieveOne( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto id = bsonValue<bsoncxx::oid>( "_id", doc );
    const auto opts = findOpts( model );

    auto client = Pool::instance().acquire();
    const auto res = (*client)[dbname][collname].find_one(
        document{} << "_id" << id << finalize, opts );
    if ( res ) return document{} << "result" << res->view() << finalize;

    LOG_WARN << "Document not found: " << dbname << ':' << collname << ':' << id.to_string();
    return model::notFound();
  }

  bsoncxx::document::view_or_value retrieve( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::basic::kvp;

    const auto doc = model.document();
    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( idopt ) return retrieveOne( model );

    const auto opts = findOpts( model );
    auto client = Pool::instance().acquire();
    auto cursor = (*client)[model.database()][model.collection()].find( doc, opts );

    auto array = bsoncxx::builder::basic::array{};
    for ( auto d : cursor ) array.append( d );
    return bsoncxx::builder::basic::make_document( kvp("results", array) );
  }

  bsoncxx::document::view_or_value create( const model::Document& document )
  {
    using spt::util::bsonValueIfExists;

    const auto doc = document.document();
    const auto dbname = document.database();
    const auto collname = document.collection();
    const auto metadata = document.metadata();

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( !idopt ) return model::missingId();

    const auto options = document.options();
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
        const auto nv = document.skipVersion();
        if ( nv && *nv )
        {
          return bsoncxx::builder::stream::document{}
              << "skipVersion" << true << bsoncxx::builder::stream::finalize;
        }
        return history( *document.bson(), client, metadata );
      }
      else
      {
        LOG_WARN << "Unable to create document " << dbname << ':' << collname << ':' << idopt->to_string();
      }
    }
    else
    {
      LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
      const auto nv = document.skipVersion();
      if ( nv && *nv )
      {
        return bsoncxx::builder::stream::document{}
            << "skipVersion" << true << bsoncxx::builder::stream::finalize;
      }
      return history( *document.bson(), client, metadata );
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

  mongocxx::options::update updateOptions( const model::Document& document )
  {
    using spt::util::bsonValueIfExists;

    const auto options = document.options();
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

  bsoncxx::document::view_or_value updateOne( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto metadata = model.metadata();
    const auto oid = bsonValue<bsoncxx::oid>( "_id", doc );
    const auto skip = model.skipVersion();

    auto opts = updateOptions( model );
    auto client = Pool::instance().acquire();
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto vhd = [&dbname, &collname, &client, &metadata, &oid, &skip]() -> bsoncxx::document::view_or_value
    {
      if ( skip && *skip ) return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

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

  bsoncxx::document::view_or_value replaceOne( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto metadata = model.metadata();
    const auto filter = bsonValue<bsoncxx::document::view>( "filter", doc );
    const auto skip = model.skipVersion();

    auto client = Pool::instance().acquire();
    const auto options = model.options();
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

    const auto vhd = [&dbname, &collname, &client, &metadata, &filter, &skip]() -> bsoncxx::document::view_or_value
    {
      if ( skip && *skip ) return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

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

  bsoncxx::document::view_or_value update( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto metadata = model.metadata();
    const auto skip = model.skipVersion();

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( idopt ) return updateOne( model );

    const auto filter = bsonValueIfExists<bsoncxx::document::view>( "filter", doc );
    if ( !filter ) return model::invalidAUpdate();

    const auto replace = bsonValueIfExists<bsoncxx::document::view>( "replace", doc );
    if ( replace ) return replaceOne( model );

    const auto update = bsonValueIfExists<bsoncxx::document::view>( "update", doc );
    if ( !update ) return model::invalidAUpdate();

    auto client = Pool::instance().acquire();
    auto opts = updateOptions( model );
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto result = (*client)[dbname][collname].update_many( *filter, updateDoc( *update ), opts );

    const auto vhd = [&dbname, &collname, &client, &filter, &metadata, &skip]() -> bsoncxx::document::view_or_value
    {
      if ( skip && *skip ) return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

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

  bsoncxx::document::view_or_value remove( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;

    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto doc = model.document();
    const auto metadata = model.metadata();
    const auto skip = model.skipVersion();

    const auto options = model.options();
    auto opts = mongocxx::options::delete_options{};
    if ( options )
    {
      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ) );

      auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( co ) opts.collation( *co );
    }

    auto client = Pool::instance().acquire();
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    auto docs = bsoncxx::builder::basic::array{};
    auto success = bsoncxx::builder::basic::array{};
    auto fail = bsoncxx::builder::basic::array{};
    auto vh = bsoncxx::builder::basic::array{};
    auto results = (*client)[dbname][collname].find( doc );

    for ( auto d : results ) docs.append( d );

    const auto rm = [&client, &dbname, &collname, &metadata, &opts, &success, &fail, &vh, &skip]( auto d )
    {
      const auto vhd = [&client, &dbname, &collname, &metadata, &vh, &skip]( auto d )
      {
        if ( skip && *skip ) return;

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

    for ( auto d : docs.view() )
    {
      const auto item = d.get_document().view();
      rm( item );
    }

    return document{} << "success" << success << "failure" << fail << "history" << vh << finalize;
  }

  bsoncxx::document::view_or_value process( const model::Document& document )
  {
    using spt::util::bsonValue;

    try
    {
      const auto action = document.action();
      if ( action == "create" )
      {
        return create( document );
      }
      else if ( action == "update" )
      {
        return update( document );
      }
      else if ( action == "retrieve" )
      {
        return retrieve( document );
      }
      else if ( action == "delete" )
      {
        return remove( document );
      }
      else if ( action == "count" )
      {
        return count( document );
      }
      else if ( action == "index" )
      {
        return index( document );
      }
      else if ( action == "dropIndex" )
      {
        return dropIndex( document );
      }

      return bsoncxx::document::value{ model::invalidAction() };
    }
    catch ( const mongocxx::bulk_write_exception& be )
    {
      std::ostringstream ss;
      ss << "Error processing database action " << document.action() <<
         " code: " << be.code() << ", message: " << be.what();
      LOG_CRIT << ss.str();
      LOG_INFO << document.json();

      std::ostringstream oss;
      oss << "Error processing database action " << document.action();
      return model::withMessage( oss.str() );
    }
    catch ( const mongocxx::logic_error& le )
    {
      std::ostringstream ss;
      ss << "Error processing database action " << document.action() <<
         " code: " << le.code() << ", message: " << le.what();
      LOG_CRIT << ss.str();
      LOG_INFO << document.json();

      std::ostringstream oss;
      oss << "Error processing database action " << document.action();
      return model::withMessage( oss.str() );
    }
    catch ( const std::exception& ex )
    {
      LOG_CRIT << "Error processing database action " << document.action() << ". " << ex.what();
      LOG_INFO << document.json();
    }

    return model::unexpectedError();
  }
}

bsoncxx::document::view_or_value spt::db::process( const model::Document& document )
{
  using util::bsonValueIfExists;

  const auto st = std::chrono::steady_clock::now();
  auto value = pstorage::process( document );
  const auto et = std::chrono::steady_clock::now();
  const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );

  auto metric = model::Metric{};
  metric.action = document.action();
  metric.database = document.database();
  metric.collection = document.collection();
  metric.duration = delta;

  auto doc = document.document();
  metric.id = bsonValueIfExists<bsoncxx::oid>( "_id", doc );

  metric.application = document.application();
  metric.correlationId = document.correlationId();
  metric.message = bsonValueIfExists<std::string>( "error", value.view() );

  auto& conf = model::Configuration::instance();
  auto client = Pool::instance().acquire();
  (*client)[conf.versionHistoryDatabase][conf.metricsCollection].insert_one( metric.bson() );

  return value;
}
