//
// Created by Rakesh on 20/07/2020.
//

#include "storage.h"
#include "pool.h"
#include "log/NanoLog.h"
#include "model/configuration.h"
#include "model/errors.h"
#include "model/metric.h"
#include "queue/queuemanager.h"
#include "util/bson.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <chrono>
#include <sstream>
#include <vector>

namespace spt::db::pstorage
{
  using boost::asio::awaitable;

  awaitable<bsoncxx::document::view_or_value> history( bsoncxx::document::view view,
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

    const auto vr = ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].insert_one( d << finalize );
    if ( client->write_concern().is_acknowledged())
    {
      if ( vr )
      {
        LOG_INFO <<
          "Created version for " << dbname << ':' << collname << ':' <<
          id.to_string() << " with id: " << oid.to_string();
        co_return document{} << "_id" << oid <<
          "database" << conf.versionHistoryDatabase <<
          "collection" << conf.versionHistoryCollection <<
          "entity" << id << finalize;
      }
      else
      {
        LOG_WARN
          << "Unable to create version for " << dbname << ':' << collname
          << ':' << id.to_string();
      }
    }
    else
    {
      LOG_INFO
        << "Created version for " << dbname << ':' << collname << ':' <<
        id.to_string() << " with id: " << oid.to_string();
      co_return document{} << "_id" << oid <<
        "database" << conf.versionHistoryDatabase <<
        "collection" << conf.versionHistoryCollection <<
        "entity" << id << finalize;
    }

    co_return model::createVersionFailed();
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

    auto majority = bsonValueIfExists<std::chrono::milliseconds>( "majority", view );
    if ( majority ) w.majority( *majority );

    auto tag = bsonValueIfExists<std::string>( "tag", view );
    if ( tag ) w.tag( *tag );

    auto timeout = bsonValueIfExists<std::chrono::milliseconds>( "timeout", view );
    if ( timeout ) w.timeout( *timeout );

    return w;
  }

  mongocxx::options::index indexOpts( const model::Document& model )
  {
    using spt::util::bsonValueIfExists;

    const auto options = model.options();
    auto opts = mongocxx::options::index{};

    if ( options )
    {
      const auto collation = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( collation ) opts.collation( *collation );

      const auto background = bsonValueIfExists<bool>( "background", *options );
      if ( background ) opts.background( *background );

      const auto unique = bsonValueIfExists<bool>( "unique", *options );
      if ( unique ) opts.unique( *unique );

      const auto hidden = bsonValueIfExists<bool>( "hidden", *options );
      if ( hidden ) opts.hidden( *hidden );

      const auto name = bsonValueIfExists<std::string>( "name", *options );
      if ( name ) opts.name( *name );

      const auto sparse = bsonValueIfExists<bool>( "sparse", *options );
      if ( sparse ) opts.sparse( *sparse );

      const auto expireAfterSeconds = bsonValueIfExists<int32_t>( "expireAfterSeconds", *options );
      if ( expireAfterSeconds ) opts.expire_after( std::chrono::seconds{ *expireAfterSeconds } );

      const auto version = bsonValueIfExists<int32_t>( "version", *options );
      if ( version ) opts.version( *version );

      const auto weights = bsonValueIfExists<bsoncxx::document::view>( "weights", *options );
      if ( weights ) opts.weights( *weights );

      const auto language_override = bsonValueIfExists<std::string>( "languageOverride", *options );
      if ( language_override ) opts.language_override( *language_override );

      const auto partial_filter_expression = bsonValueIfExists<bsoncxx::document::view>( "partialFilterExpression", *options );
      if ( partial_filter_expression ) opts.partial_filter_expression( *partial_filter_expression );

      const auto twod_sphere_version = bsonValueIfExists<int32_t>( "twodSphereVersion", *options );
      if ( twod_sphere_version ) opts.twod_sphere_version( static_cast<uint8_t>( *twod_sphere_version ) );

      const auto twod_bits_precision = bsonValueIfExists<int32_t>( "twodBitsPrecision", *options );
      if ( twod_bits_precision ) opts.twod_bits_precision( static_cast<uint8_t>( *twod_bits_precision ) );

      const auto twod_location_min = bsonValueIfExists<double>( "twodLocationMin", *options );
      if ( twod_location_min ) opts.twod_location_min( *twod_location_min );

      const auto twod_location_max = bsonValueIfExists<double>( "twodLocationMax", *options );
      if ( twod_location_max ) opts.twod_location_max( *twod_location_max );
    }

    return opts;
  }

  awaitable<bsoncxx::document::view_or_value> index( const model::Document& model )
  {
    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto options = model.options();

    LOG_INFO << "Creating index " << model.json();
    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    co_return options ?
        ( *client )[dbname][collname].create_index( doc, indexOpts( model ) ) :
        ( *client )[dbname][collname].create_index( doc );
  }

  awaitable<bsoncxx::document::view_or_value> dropIndex( const model::Document& model )
  {
    using util::bsonValueIfExists;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto options = model.options();

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;

    const auto name = bsonValueIfExists<std::string>( "name", doc );
    if ( name )
    {
      try
      {
        ( *client )[dbname][collname].indexes().drop_one( *name );
      }
      catch ( const mongocxx::exception& ex )
      {
        LOG_WARN << "Error dropping index " << *name << ". " << ex.what();
        std::string m{ ex.what() };
        co_return model::withMessage( m );
      }
    }

    const auto spec = bsonValueIfExists<bsoncxx::document::view>( "specification", doc );
    if ( spec )
    {
      try
      {
        if ( options )
        {
          ( *client )[dbname][collname].indexes().drop_one(
              *spec, indexOpts( model ) );
        }
        else ( *client )[dbname][collname].indexes().drop_one( *spec );
      }
      catch ( const mongocxx::exception& ex )
      {
        LOG_WARN << "Error dropping index " << *name << ". " << ex.what() << ". " << bsoncxx::to_json( *spec );
        std::string m{ ex.what() };
        co_return model::withMessage( m );
      }
    }

    co_return document{} << "dropIndex" << true << finalize;
  }

  awaitable<bsoncxx::document::view_or_value> dropCollection( const model::Document& model )
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
    co_return document{} << "dropCollection" << true << finalize;
  }

  awaitable<bsoncxx::document::view_or_value> count( const model::Document& model )
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

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    const auto count = ( *client )[dbname][collname].count_documents(
        model.document(), options );
    co_return document{} << "count" << count << finalize;
  }

  mongocxx::options::find findOpts( const model::Document& model )
  {
    using spt::util::bsonValueIfExists;

    const auto options = model.options();
    auto opts = mongocxx::options::find{};

    if ( options )
    {
      const auto partial = bsonValueIfExists<bool>( "partialResults",
          *options );
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

      const auto maxTime = bsonValueIfExists<std::chrono::milliseconds>(
          "maxTime", *options );
      if ( maxTime ) opts.max_time( *maxTime );

      const auto min = bsonValueIfExists<bsoncxx::document::view>( "min", *options );
      if ( min ) opts.min( *min );

      const auto projection = bsonValueIfExists<bsoncxx::document::view>(
          "projection", *options );
      if ( projection ) opts.projection( *projection );

      const auto rp = bsonValueIfExists<int32_t>( "readPreference", *options );
      if ( rp )
      {
        auto p = mongocxx::read_preference{};
        p.mode( static_cast<mongocxx::read_preference::read_mode>( *rp ));
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

  awaitable<bsoncxx::document::view_or_value> retrieveOne( const model::Document& model )
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

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    const auto res = ( *client )[dbname][collname].find_one( doc, opts );
    if ( res ) co_return document{} << "result" << res->view() << finalize;

    LOG_WARN << "Document not found: " << dbname << ':' << collname << ':'
      << id.to_string() << ". " << bsoncxx::to_json( doc );
    co_return model::notFound();
  }

  awaitable<bsoncxx::document::view_or_value> retrieve( const model::Document& model )
  {
    using spt::util::bsonValue;
    using spt::util::bsonValueIfExists;

    using bsoncxx::builder::basic::kvp;

    const auto doc = model.document();
    if ( doc.find( "_id" ) != doc.end() )
    {
      if ( bsoncxx::v_noabi::type::k_oid == doc["_id"].type() )
      {
        LOG_DEBUG << "_id property is of type oid";
        co_return co_await retrieveOne( model );
      }
    }

    const auto opts = findOpts( model );
    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    auto cursor = ( *client )[model.database()][model.collection()].find( doc, opts );

    auto array = bsoncxx::builder::basic::array{};
    for ( auto&& d : cursor ) array.append( d );
    co_return bsoncxx::builder::basic::make_document( kvp( "results", array ));
  }

  mongocxx::options::insert insertOpts( const model::Document& document )
  {
    using spt::util::bsonValueIfExists;

    const auto options = document.options();
    auto opts = mongocxx::options::insert{};
    if ( options )
    {
      auto validate = bsonValueIfExists<bool>( "bypassValidation", *options );
      if ( validate ) opts.bypass_document_validation( *validate );

      auto ordered = bsonValueIfExists<bool>( "ordered", *options );
      if ( ordered ) opts.ordered( *ordered );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern",
          *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ));
    }

    return opts;
  }

  awaitable<bsoncxx::document::view_or_value> create( const model::Document& document )
  {
    using spt::util::bsonValueIfExists;

    const auto doc = document.document();
    const auto dbname = document.database();
    const auto collname = document.collection();
    const auto metadata = document.metadata();

    const auto& conf = model::Configuration::instance();
    if ( dbname == conf.versionHistoryDatabase && collname == conf.versionHistoryCollection )
    {
      LOG_WARN << "Attempting to create in version history " << document.json();
      co_return model::notModifyable();
    }

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( !idopt ) co_return model::missingId();

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;

    auto opts = insertOpts( document );
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto result = ( *client )[dbname][collname].insert_one( doc, opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Created document " << dbname << ':' << collname << ':'
          << idopt->to_string();
        const auto nv = document.skipVersion();
        if ( nv && *nv )
        {
          co_return bsoncxx::builder::stream::document{} <<
            "_id" << *idopt <<
            "skipVersion" << true << bsoncxx::builder::stream::finalize;
        }
        co_return co_await history( *document.bson(), client, metadata );
      }
      else
      {
        LOG_WARN << "Unable to create document " << dbname << ':' << collname
          << ':' << idopt->to_string();
      }
    }
    else
    {
      LOG_INFO << "Created document " << dbname << ':' << collname << ':'
        << idopt->to_string();
      const auto nv = document.skipVersion();
      if ( nv && *nv )
      {
        co_return bsoncxx::builder::stream::document{} <<
          "_id" << *idopt <<
          "skipVersion" << true << bsoncxx::builder::stream::finalize;
      }
      co_return co_await history( *document.bson(), client, metadata );
    }

    co_return model::insertError();
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

    for ( auto&& e : doc )
    {
      if ( e.key() == "_id" ) continue;

      switch ( e.type() )
      {
      case bsoncxx::type::k_array:d << e.key() << e.get_array();
        break;
      case bsoncxx::type::k_bool:d << e.key() << e.get_bool();
        break;
      case bsoncxx::type::k_date:d << e.key() << e.get_date();
        break;
      case bsoncxx::type::k_decimal128:d << e.key() << e.get_decimal128();
        break;
      case bsoncxx::type::k_document:d << e.key() << e.get_document();
        break;
      case bsoncxx::type::k_double:d << e.key() << e.get_double();
        break;
      case bsoncxx::type::k_int32:d << e.key() << e.get_int32();
        break;
      case bsoncxx::type::k_int64:d << e.key() << e.get_int64();
        break;
      case bsoncxx::type::k_oid:d << e.key() << e.get_oid();
        break;
      case bsoncxx::type::k_null:d << e.key() << e.get_null();
        break;
      case bsoncxx::type::k_timestamp:d << e.key() << e.get_timestamp();
        break;
      case bsoncxx::type::k_utf8:
        d << e.key() << bsonValue<std::string>( e.key(), doc );
        break;
      default:
        LOG_WARN << "Un-mapped bson type: " << bsoncxx::to_string( e.type());
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

      auto col = bsonValueIfExists<bsoncxx::document::view>( "collation",
          *options );
      if ( col ) opts.collation( *col );

      auto upsert = bsonValueIfExists<bool>( "upsert", *options );
      if ( upsert ) opts.upsert( *upsert );

      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern",
          *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ));

      auto af = bsonValueIfExists<bsoncxx::array::view>( "arrayFilters",
          *options );
      if ( af ) opts.array_filters( *af );
    }

    return opts;
  }

  awaitable<bsoncxx::document::view_or_value> updateOne( const model::Document& model )
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
    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    if ( !opts.write_concern()) opts.write_concern( client->write_concern());

    const auto vhd = [&]() -> awaitable<bsoncxx::document::view_or_value>
    {
      if ( skip && *skip ) co_return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

      const auto updated = ( *client )[dbname][collname].find_one(
          document{} << "_id" << oid << finalize );
      if ( !updated ) co_return model::notFound();
      if ( bsonValueIfExists<std::string>( "error", *updated ) )
      {
        co_return bsoncxx::document::value{ updated.value() };
      }

      auto vhd = co_await history( document{} << "action" << "update" <<
        "database" << dbname <<
        "collection" << collname <<
        "document" << updated->view()
        << finalize, client, metadata );
      if ( bsonValueIfExists<std::string>( "error", vhd ) ) co_return vhd;

      co_return bsoncxx::document::view_or_value{
        document{} << "document" << updated->view() << "history" << vhd
        << finalize };
    };

    const auto result = ( *client )[dbname][collname].update_one(
        document{} << "_id" << oid << finalize, updateDoc( doc ), opts );
    if ( opts.write_concern()->is_acknowledged())
    {
      if ( result )
      {
        LOG_INFO << "Updated document " << dbname << ':' << collname << ':'
          << oid.to_string();
        co_return co_await vhd();
      }
      else
      {
        LOG_WARN << "Unable to update document " << dbname << ':' << collname
          << ':' << oid.to_string();
      }
    }
    else
    {
      LOG_INFO << "Updated document " << dbname << ':' << collname << ':'
        << oid.to_string();
      co_return co_await vhd();
    }

    co_return model::updateError();
  }

  awaitable<bsoncxx::document::view_or_value> updateOneByFilter(
      const model::Document& model, const bsoncxx::oid& oid )
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
    const auto filter = bsonValue<bsoncxx::document::view>( "filter", doc );

    auto opts = updateOptions( model );
    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    if ( !opts.write_concern()) opts.write_concern( client->write_concern());

    const auto vhd = [&]() -> awaitable<bsoncxx::document::view_or_value>
    {
      if ( skip && *skip ) co_return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

      const auto updated = ( *client )[dbname][collname].find_one( filter );
      if ( !updated ) co_return model::notFound();
      if ( bsonValueIfExists<std::string>( "error", *updated ) )
      {
        co_return bsoncxx::document::value{ updated.value() };
      }

      auto vhd = co_await history( document{} << "action" << "update" <<
        "database" << dbname <<
        "collection" << collname <<
        "document" << updated->view()
        << finalize, client, metadata );
      if ( bsonValueIfExists<std::string>( "error", vhd ) ) co_return vhd;

      co_return bsoncxx::document::view_or_value{
          document{} << "document" << updated->view() << "history" << vhd << finalize };
    };

    const auto upd = bsonValue<bsoncxx::document::view>( "update", doc );
    const auto result = ( *client )[dbname][collname].update_one( filter, updateDoc( upd ), opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
        co_return co_await vhd();
      }
      else
      {
        LOG_WARN << "Unable to update document " << dbname << ':' << collname
          << ':' << oid.to_string();
      }
    }
    else
    {
      LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
      co_return co_await vhd();
    }

    co_return model::updateError();
  }

  awaitable<bsoncxx::document::view_or_value> replaceOne( const model::Document& model )
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
    const auto replace = bsonValue<bsoncxx::document::view>( "replace", doc );
    const auto oid = bsonValueIfExists<bsoncxx::oid>( "_id", replace );
    const auto skip = model.skipVersion();

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
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
      if ( wc ) opts.write_concern( writeConcern( *wc ));
    }

    const auto vhd = [&]() -> awaitable<bsoncxx::document::view_or_value>
    {
      if ( skip && *skip )
      {
        co_return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;
      }

      if ( oid )
      {
        auto vhd = co_await history( document{} <<
          "action" << "replace" <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << replace <<
          finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", vhd ) ) co_return vhd;

        co_return document{} << "document" << replace << "history" << vhd << finalize;
      }
      else
      {
        const auto updated = ( *client )[dbname][collname].find_one( filter );
        if ( !updated )
        {
          LOG_WARN << "Updated document not found in " <<
            dbname << ':' << collname << " by filter " << bsoncxx::to_json( filter );
          co_return model::notFound();
        }

        auto vhd = co_await history( document{} <<
          "action" << "replace" <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << updated->view() <<
          finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", vhd ) ) co_return vhd;

        co_return document{} << "document" << updated->view() << "history" << vhd << finalize;
      }
    };

    if ( !opts.write_concern()) opts.write_concern( client->write_concern() );
    const auto result = ( *client )[dbname][collname].replace_one( filter, replace, opts );
    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Updated document in " << dbname << ':' << collname <<
          " with filter " << bsoncxx::to_json( filter );
        co_return co_await vhd();
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
      co_return co_await vhd();
    }

    co_return model::updateError();
  }

  awaitable<bsoncxx::document::view_or_value> update( const model::Document& model )
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

    const auto& conf = model::Configuration::instance();
    if ( dbname == conf.versionHistoryDatabase && collname == conf.versionHistoryCollection )
    {
      LOG_WARN << "Attempting to update in version history " << model.json();
      co_return model::notModifyable();
    }

    const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );
    if ( idopt ) co_return co_await updateOne( model );

    const auto filter = bsonValueIfExists<bsoncxx::document::view>( "filter", doc );
    if ( !filter ) co_return model::invalidAUpdate();

    const auto replace = bsonValueIfExists<bsoncxx::document::view>( "replace", doc );
    if ( replace ) co_return co_await replaceOne( model );

    const auto update = bsonValueIfExists<bsoncxx::document::view>( "update", doc );
    if ( !update ) co_return model::invalidAUpdate();

    auto iter = filter->find( "_id" );
    if ( iter != filter->end() )
    {
      if ( iter->type() == bsoncxx::type::k_oid )
      {
        const auto fid = iter->get_oid().value;
        co_return co_await updateOneByFilter( model, fid );
      }
    }

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    auto opts = updateOptions( model );
    if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

    const auto result = ( *client )[dbname][collname].update_many(
        *filter, updateDoc( *update ), opts );

    const auto vhd = [&]() -> awaitable<bsoncxx::document::view_or_value>
    {
      if ( skip && *skip ) co_return document{} << "skipVersion" << true << bsoncxx::builder::stream::finalize;

      auto success = bsoncxx::builder::basic::array{};
      auto fail = bsoncxx::builder::basic::array{};
      auto vh = bsoncxx::builder::basic::array{};
      auto results = ( *client )[dbname][collname].find( *filter );

      auto docs = bsoncxx::builder::basic::array{};
      for ( auto&& d : results ) docs.append( d );

      for ( auto&& d : docs.view() )
      {
        auto vhd = co_await history( document{} << "action" << "update" <<
          "database" << dbname << "collection" << collname <<
          "document" << d.get_document().view()
          << finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", vhd ) )
          fail.append( d["_id"].get_oid() );
        else
        {
          success.append( d["_id"].get_oid() );
          vh.append( vhd );
        }
      }

      co_return document{} << "success" << success << "failure" << fail
        << "history" << vh << finalize;
    };

    if ( opts.write_concern()->is_acknowledged() )
    {
      if ( result )
      {
        LOG_INFO << "Created document " << dbname << ':' << collname << ':'
          << idopt->to_string();
        co_return co_await vhd();
      }
      else
      {
        LOG_WARN << "Unable to create document " << dbname << ':' << collname
          << ':' << idopt->to_string();
      }
    }
    else
    {
      LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
      co_return co_await vhd();
    }

    co_return model::updateError();
  }

  awaitable<bsoncxx::document::view_or_value> remove( const model::Document& model )
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

    const auto& conf = model::Configuration::instance();
    if ( dbname == conf.versionHistoryDatabase && collname == conf.versionHistoryCollection )
    {
      LOG_WARN << "Attempting to delete from version history " << model.json();
      co_return model::notModifyable();
    }

    const auto options = model.options();
    auto opts = mongocxx::options::delete_options{};
    if ( options )
    {
      auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options );
      if ( wc ) opts.write_concern( writeConcern( *wc ));

      auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options );
      if ( co ) opts.collation( *co );
    }

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    if ( !opts.write_concern()) opts.write_concern( client->write_concern());

    auto docs = bsoncxx::builder::basic::array{};
    auto success = bsoncxx::builder::basic::array{};
    auto fail = bsoncxx::builder::basic::array{};
    auto vh = bsoncxx::builder::basic::array{};
    auto results = ( *client )[dbname][collname].find( doc );

    for ( auto&& d : results ) docs.append( d );

    const auto rm = [&]( const auto& d ) -> awaitable<bool>
    {
      const auto vhd = [&]( const auto& d ) -> awaitable<bool>
      {
        if ( skip && *skip ) co_return true;

        auto vhd = co_await history( document{} << "action" << "delete" <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << d << finalize,
          client, metadata );
        vh.append( vhd );
        co_return true;
      };

      const auto oid = bsonValue<bsoncxx::oid>( "_id", d );
      const auto res = ( *client )[dbname][collname].delete_one(
          document{} << "_id" << oid << finalize, opts );

      if ( opts.write_concern()->is_acknowledged())
      {
        if ( res )
        {
          LOG_INFO << "Deleted document " << dbname << ':' << collname << ':' << oid.to_string();
          success.append( oid );
          co_await vhd( d );
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
        co_await vhd( d );
      }

      co_return true;
    };

    for ( auto&& d : docs.view() )
    {
      const auto item = d.get_document().view();
      co_await rm( item );
    }

    co_return document{} << "success" << success << "failure" << fail
      << "history" << vh << finalize;
  }

  awaitable<bsoncxx::document::view_or_value> bulk( const model::Document& model )
  {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using spt::util::bsonValueIfExists;

    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();
    const auto metadata = model.metadata();
    const auto skip = model.skipVersion();

    const auto insert = bsonValueIfExists<bsoncxx::array::view>( "insert", doc );
    auto icount = 0;
    auto ihcount = 0;
    const auto rem = bsonValueIfExists<bsoncxx::array::view>( "delete", doc );
    auto rcount = 0;

    if ( !insert && !rem ) co_return model::withMessage( "Bulk insert missing arrays." );

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    auto& client = *cliento;
    const auto& conf = model::Configuration::instance();
    auto bw = ( *client )[dbname][collname].create_bulk_write();
    auto bwh = ( *client )[conf.versionHistoryDatabase][conf.versionHistoryCollection].create_bulk_write();
    std::vector<bsoncxx::document::value> histv;

    if ( insert )
    {
      for ( auto e : *insert )
      {
        const auto dv = e.get_document().view();
        const auto oid = util::bsonValueIfExists<bsoncxx::oid>( "_id", dv );
        if ( oid )
        {
          bw.append( mongocxx::model::insert_one{ dv } );
          ++icount;
        }
      }

      if ( !skip || !*skip )
      {
        histv.reserve( icount );

        for ( auto e : *insert )
        {
          const auto dv = e.get_document().view();
          const auto oid = util::bsonValueIfExists<bsoncxx::oid>( "_id", dv );
          if ( oid )
          {
            auto d = document{};
            d << "_id" << bsoncxx::oid{} <<
              "database" << dbname <<
              "collection" << collname <<
              "action" << "create" <<
              "entity" << dv <<
              "created" << bsoncxx::types::b_date{ std::chrono::system_clock::now() };
            if ( metadata ) d << "metadata" << *metadata;
            histv.emplace_back( d << finalize );
            bwh.append( mongocxx::model::insert_one{ histv.back().view() } );
            ++ihcount;
          }
        }
      }
    }

    if ( rem )
    {
      for ( auto e : *rem )
      {
        bw.append( mongocxx::model::delete_one{ e.get_document().view() } );
        ++rcount;
      }

      if ( !skip || !*skip )
      {
        if ( histv.empty() ) histv.reserve( rcount );

        for ( auto e : *rem )
        {
          auto res = ( *client )[dbname][collname].find( e.get_document().view() );
          for ( auto&& d : res )
          {
            auto vhd = document{};
            vhd << "_id" << bsoncxx::oid{} <<
              "database" << dbname <<
              "collection" << collname <<
              "action" << "delete" <<
              "entity" << d <<
              "created" << bsoncxx::types::b_date{ std::chrono::system_clock::now() };
            if ( metadata ) vhd << "metadata" << *metadata;
            histv.emplace_back( vhd << finalize );
            bwh.append( mongocxx::model::insert_one{ histv.back().view() } );
            ++ihcount;
          }
        }
      }
    }

    auto r = bw.execute();
    auto ihc = 0;
    if ( !skip || !*skip )
    {
      if ( !histv.empty() )
      {
        auto rh = bwh.execute();
        if ( client->write_concern().is_acknowledged() )
        {
          ihc = rh->inserted_count();
        }
      }
    }

    if ( client->write_concern().is_acknowledged() )
    {
      if ( !r )
      {
        LOG_WARN << "Error executing bulk statements";
        co_return model::withMessage( "Error executing bulk statements." );
      }

      co_return document{} <<
        "create" << r->inserted_count() <<
        "history" << ihc <<
        "delete" << r->deleted_count() << finalize;
    }

    co_return document{} <<
      "create" << icount <<
      "history" << ihcount <<
      "delete" << rcount <<
      finalize;
  }

  awaitable<bsoncxx::document::view_or_value> pipeline( const model::Document& model )
  {
    using spt::util::bsonValueIfExists;

    LOG_DEBUG << "Executing aggregation pipeline query";
    const auto doc = model.document();
    const auto dbname = model.database();
    const auto collname = model.collection();

    const auto match = bsonValueIfExists<bsoncxx::document::view>( "match", doc );
    if ( ! match )
    {
      LOG_WARN << "No match document specified";
      co_return model::withMessage( "No match document in payload." );
    }

    const auto group = bsonValueIfExists<bsoncxx::document::view>( "group", doc );
    if ( ! group )
    {
      LOG_WARN << "No group document specified";
      co_return model::withMessage( "No group document in payload." );
    }

    auto pipeline = mongocxx::pipeline{};
    pipeline.match( *match );
    pipeline.group( *group );

    auto cliento = Pool::instance().acquire();
    if ( !cliento )
    {
      LOG_WARN << "Connection pool exhausted";
      co_return model::poolExhausted();
    }

    const auto& client = *cliento;
    auto aggregate = ( *client )[dbname][collname].aggregate( pipeline );

    auto array = bsoncxx::builder::basic::array{};
    for ( auto&& d : aggregate ) array.append( d );
    co_return bsoncxx::builder::basic::make_document( kvp( "results", array ) );
  }

  boost::asio::awaitable<bsoncxx::document::view_or_value> process(
      const model::Document& document )
  {
    using spt::util::bsonValue;

    try
    {
      const auto action = document.action();
      if ( action == "create" )
      {
        co_return co_await create( document );
      }
      else if ( action == "update" )
      {
        co_return co_await update( document );
      }
      else if ( action == "retrieve" )
      {
        co_return co_await retrieve( document );
      }
      else if ( action == "delete" )
      {
        co_return co_await remove( document );
      }
      else if ( action == "count" )
      {
        co_return co_await count( document );
      }
      else if ( action == "index" )
      {
        co_return co_await index( document );
      }
      else if ( action == "dropIndex" )
      {
        co_return co_await dropIndex( document );
      }
      else if ( action == "dropCollection" )
      {
        co_return co_await dropCollection( document );
      }
      else if ( action == "bulk" )
      {
        co_return co_await bulk( document );
      }
      else if ( action == "pipeline" )
      {
        co_return co_await pipeline( document );
      }
      else if ( action == "transaction" )
      {
        co_return co_await internal::transaction( document );
      }

      co_return model::invalidAction();
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
      co_return model::withMessage( oss.str() );
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
      co_return model::withMessage( oss.str() );
    }
    catch ( const std::exception& ex )
    {
      LOG_CRIT << "Error processing database action " << document.action() << ". " << ex.what();
      LOG_INFO << document.json();
    }

    co_return model::unexpectedError();
  }
}

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::process(
    const model::Document& document )
{
  using util::bsonValueIfExists;

  const auto st = std::chrono::steady_clock::now();
  auto value = co_await pstorage::process( document );
  const auto et = std::chrono::steady_clock::now();
  const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );

  auto metric = model::Metric{};
  metric.action = document.action();
  metric.database = document.database();
  metric.collection = document.collection();
  metric.duration = delta;

  auto doc = document.document();
  if ( doc.find( "_id" ) != doc.end() )
  {
    if ( bsoncxx::type::k_oid == doc["_id"].type() )
    {
      metric.id = util::bsonValue<bsoncxx::oid>( "_id", doc );
    }
  }

  metric.application = document.application();
  metric.correlationId = document.correlationId();
  metric.message = bsonValueIfExists<std::string>( "error", value.view() );
  metric.size = value.view().length();
  queue::QueueManager::instance().publish( std::move( metric ) );

  co_return value;
}
