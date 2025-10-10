//
// Created by Rakesh on 20/07/2020.
//

#include "metricscollector.hpp"
#include "storage.hpp"
#include "pool.hpp"
#include "internal/internal.hpp"
#include "model/configuration.hpp"
#include "model/errors.hpp"
#include "model/metric.hpp"
#include "../log/NanoLog.hpp"
#include "../common/util/bson.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <chrono>
#include <format>
#include <ranges>
#include <vector>

namespace
{
  namespace pstorage
  {
    using namespace spt;
    using namespace spt::db;
    using boost::asio::awaitable;

    awaitable<bsoncxx::document::view_or_value> history( bsoncxx::document::view view, mongocxx::pool::entry& client,
        std::optional<bsoncxx::document::view> metadata = std::nullopt )
    {
      using util::bsonValue;

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
      if ( client->write_concern().is_acknowledged() )
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

    bsoncxx::document::value indexOpts( const model::Document& model )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;
      using util::bsonValueIfExists;

      const auto options = model.options();
      auto opts = mongocxx::options::index{};

      if ( options )
      {
        if ( const auto collation = bsonValueIfExists<bsoncxx::document::view>( "collation", *options ); collation ) opts.collation( *collation );
        if ( const auto background = bsonValueIfExists<bool>( "background", *options ); background ) opts.background( *background );
        if ( const auto unique = bsonValueIfExists<bool>( "unique", *options ); unique ) opts.unique( *unique );
        if ( const auto hidden = bsonValueIfExists<bool>( "hidden", *options ); hidden ) opts.hidden( *hidden );
        if ( const auto name = bsonValueIfExists<std::string>( "name", *options ); name ) opts.name( *name );
        if ( const auto sparse = bsonValueIfExists<bool>( "sparse", *options ); sparse ) opts.sparse( *sparse );
        if ( const auto exp = bsonValueIfExists<std::chrono::seconds>( "expireAfterSeconds", *options ); exp ) opts.expire_after( *exp );
        if ( const auto version = bsonValueIfExists<int32_t>( "version", *options ); version ) opts.version( *version );
        if ( const auto weights = bsonValueIfExists<bsoncxx::document::view>( "weights", *options ); weights ) opts.weights( *weights );
        if ( const auto lang = bsonValueIfExists<std::string>( "defaultLanguage", *options ); lang ) opts.default_language( *lang );
        if ( const auto lo = bsonValueIfExists<std::string>( "languageOverride", *options ); lo ) opts.language_override( *lo );
        if ( const auto pfe = bsonValueIfExists<bsoncxx::document::view>( "partialFilterExpression", *options ); pfe ) opts.partial_filter_expression( *pfe );
        if ( const auto td = bsonValueIfExists<int32_t>( "twodSphereVersion", *options ); td ) opts.twod_sphere_version( static_cast<uint8_t>( *td ) );
        if ( const auto tbp = bsonValueIfExists<int32_t>( "twodBitsPrecision", *options ); tbp ) opts.twod_bits_precision( static_cast<uint8_t>( *tbp ) );
        if ( const auto tlm = bsonValueIfExists<double>( "twodLocationMin", *options ); tlm ) opts.twod_location_min( *tlm );
        if ( const auto tlm = bsonValueIfExists<double>( "twodLocationMax", *options ); tlm ) opts.twod_location_max( *tlm );
      }

      bsoncxx::document::view_or_value idv = opts;
      auto builder = document{};
      for ( const auto& e : idv.view() ) builder << e.key() << e.get_value();

      if ( const auto v = bsonValueIfExists<int32_t>( "textIndexVersion", *options ); v ) builder << "textIndexVersion" << *v;
      if ( const auto v = bsonValueIfExists<bsoncxx::document::view>( "wildcardProjection", *options ); v ) builder << "wildcardProjection" << *v;

      return builder << finalize;
    }

    awaitable<bsoncxx::document::view_or_value> index( const model::Document& model )
    {
      using util::bsonValueIfExists;

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
        ( *client )[dbname][collname].create_index( doc, indexOpts( model ) ):
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

      if ( const auto name = bsonValueIfExists<std::string>( "name", doc ); name )
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

      if ( const auto spec = bsonValueIfExists<bsoncxx::document::view>( "specification", doc ); spec )
      {
        try
        {
          if ( options )
          {
            ( *client )[dbname][collname].indexes().drop_one( *spec, indexOpts( model ) );
          }
          else ( *client )[dbname][collname].indexes().drop_one( *spec );
        }
        catch ( const mongocxx::exception& ex )
        {
          LOG_WARN << "Error dropping index. " << ex.what() << ". " << bsoncxx::to_json( *spec );
          co_return model::withMessage( ex.what() );
        }
      }

      co_return document{} << "dropIndex" << true << finalize;
    }

    mongocxx::read_preference readPreference( bsoncxx::document::view opts )
    {
      using util::bsonValueIfExists;

      auto p = mongocxx::read_preference{};

      if ( const auto v = bsonValueIfExists<bsoncxx::document::view>( "tags", opts ); v ) p.tags( *v );
      if ( const auto v = bsonValueIfExists<int64_t>( "maxStaleness", opts ); v ) p.max_staleness( std::chrono::seconds{ *v } );
      if ( const auto v = bsonValueIfExists<int32_t>( "mode", opts ); v ) p.mode( static_cast<mongocxx::read_preference::read_mode>( *v ) );
      return p;
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
        if ( const auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *opts ); col ) options.collation( *col );
        if ( const auto hint = bsonValueIfExists<bsoncxx::document::view>( "hint", *opts ); hint ) options.hint( mongocxx::hint{ *hint } );
        if ( const auto limit = bsonValueIfExists<int64_t>( "limit", *opts ); limit ) options.limit( *limit );
        if ( const auto time = bsonValueIfExists<std::chrono::milliseconds>( "maxTime", *opts ); time ) options.max_time( *time );
        if ( const auto skip = bsonValueIfExists<int64_t>( "skip", *opts ); skip ) options.skip( *skip );
        if ( const auto rp = bsonValueIfExists<bsoncxx::document::view>( "readPreference", *opts ); rp ) options.read_preference( readPreference(  *rp ) );
      }

      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        co_return model::poolExhausted();
      }

      auto& client = *cliento;
      const auto count = ( *client )[dbname][collname].count_documents( model.document(), options );
      co_return document{} << "count" << count << finalize;
    }

    awaitable<bsoncxx::document::view_or_value> distinct( const model::Document& model )
    {
      using util::bsonValue;
      using util::bsonValueIfExists;
      using bsoncxx::builder::stream::array;
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;

      const auto dbname = model.database();
      const auto collname = model.collection();
      const auto opts = model.options();
      const auto doc = model.document();

      if ( !bsonValueIfExists<std::string>( "field", doc ) ) co_return model::missingName();

      auto options = mongocxx::options::distinct{};
      if ( opts )
      {
        if ( const auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *opts ); col ) options.collation( *col );
        if ( const auto time = bsonValueIfExists<std::chrono::milliseconds>( "maxTime", *opts ); time ) options.max_time( *time );
        if ( const auto rp = bsonValueIfExists<bsoncxx::document::view>( "readPreference", *opts ); rp ) options.read_preference( readPreference(  *rp ) );
      }

      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        co_return model::poolExhausted();
      }

      auto filter = bsonValueIfExists<bsoncxx::document::view>( "filter", doc );

      auto& client = *cliento;
      auto cursor = ( *client )[dbname][collname].distinct( bsonValue<std::string>( "field", doc ),
        filter ? *filter : document{} << finalize, options );

      auto arr = array{};
      for ( auto&& d : cursor ) arr << d;
      co_return document{} << "results" << ( arr << finalize ) << finalize;
    }

    mongocxx::options::find findOpts( const model::Document& model )
    {
      using util::bsonValueIfExists;

      const auto options = model.options();
      auto opts = mongocxx::options::find{};

      if ( options )
      {
        if ( const auto partial = bsonValueIfExists<bool>( "partialResults", *options ); partial ) opts.allow_partial_results( *partial );
        if ( const auto size = bsonValueIfExists<int32_t>( "batchSize", *options ); size ) opts.batch_size( *size );
        if ( const auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options ); co ) opts.collation( *co );
        if ( const auto com = bsonValueIfExists<std::string>( "comment", *options ); com ) opts.comment( *com );
        if ( const auto com = bsonValueIfExists<bsoncxx::document::view>( "commentOption", *options ); com ) opts.comment_option( { *com } );
        if ( const auto hint = bsonValueIfExists<bsoncxx::document::view>( "hint", *options ); hint ) opts.hint( { *hint } );
        if ( const auto let = bsonValueIfExists<bsoncxx::document::view>( "let", *options ); let ) opts.let( *let );
        if ( const auto limit = bsonValueIfExists<int64_t>( "limit", *options ); limit ) opts.limit( *limit );
        if ( const auto max = bsonValueIfExists<bsoncxx::document::view>( "max", *options ); max ) opts.max( *max );
        if ( const auto maxTime = bsonValueIfExists<std::chrono::milliseconds>( "maxTime", *options ); maxTime ) opts.max_time( *maxTime );
        if ( const auto min = bsonValueIfExists<bsoncxx::document::view>( "min", *options ); min ) opts.min( *min );
        if ( const auto projection = bsonValueIfExists<bsoncxx::document::view>( "projection", *options ); projection ) opts.projection( *projection );
        if ( const auto rp = bsonValueIfExists<bsoncxx::document::view>( "readPreference", *options ); rp ) opts.read_preference( readPreference(  *rp ) );
        if ( const auto rk = bsonValueIfExists<bool>( "returnKey", *options ); rk ) opts.return_key( *rk );
        if ( const auto ri = bsonValueIfExists<bool>( "showRecordId", *options ); ri ) opts.show_record_id( *ri );
        if ( const auto skip = bsonValueIfExists<int64_t>( "skip", *options ); skip ) opts.skip( *skip );
        if ( const auto sort = bsonValueIfExists<bsoncxx::document::view>( "sort", *options ); sort ) opts.sort( *sort );
      }

      return opts;
    }

    awaitable<bsoncxx::document::view_or_value> retrieveOne( const model::Document& model )
    {
      using util::bsonValue;
      using util::bsonValueIfExists;
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
      using util::bsonValue;
      using util::bsonValueIfExists;

      using bsoncxx::builder::stream::array;
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;

      const auto doc = model.document();
      if ( doc.find( "_id" ) != doc.end() && bsoncxx::v_noabi::type::k_oid == doc["_id"].type() )
      {
        LOG_DEBUG << "_id property is of type oid";
        co_return co_await retrieveOne( model );
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

      auto arr = array{};
      for ( auto&& d : cursor ) if ( std::ranges::distance( d ) > 0 ) arr << d;
      co_return document{} << "results" << ( arr << finalize ) << finalize;
    }

    mongocxx::options::insert insertOpts( const model::Document& document )
    {
      using util::bsonValueIfExists;

      const auto options = document.options();
      auto opts = mongocxx::options::insert{};
      if ( options )
      {
        if ( auto validate = bsonValueIfExists<bool>( "bypassValidation", *options ); validate ) opts.bypass_document_validation( *validate );
        if ( auto ordered = bsonValueIfExists<bool>( "ordered", *options ); ordered ) opts.ordered( *ordered );
        if ( auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options ); wc ) opts.write_concern( internal::writeConcern( *wc ) );
      }

      return opts;
    }

    awaitable<bsoncxx::document::view_or_value> create( const model::Document& document )
    {
      using util::bsonValueIfExists;

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
          LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
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
        LOG_INFO << "Created document " << dbname << ':' << collname << ':' << idopt->to_string();
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

    awaitable<bsoncxx::document::view_or_value> createTimeseries( const model::Document& document )
    {
      using util::bsonValueIfExists;

      const auto doc = document.document();
      const auto dbname = document.database();
      const auto collname = document.collection();

      const auto& conf = model::Configuration::instance();
      if ( dbname == conf.versionHistoryDatabase && collname == conf.versionHistoryCollection )
      {
        LOG_WARN << "Attempting to create in version history " << document.json();
        co_return model::notModifyable();
      }

      const auto idopt = bsonValueIfExists<bsoncxx::oid>( "_id", doc );

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
      auto resp = bsoncxx::builder::stream::document{};
      resp <<
        "database" << dbname <<
        "collection" << collname;

      if ( opts.write_concern()->is_acknowledged() )
      {
        if ( !result ) co_return model::insertError();

        if ( idopt ) resp << "_id" << *idopt;
        else resp << "_id" << result->inserted_id();
        co_return resp << bsoncxx::builder::stream::finalize;
      }

      co_return result ? resp << bsoncxx::builder::stream::finalize : model::insertError();
    }

    bsoncxx::document::value updateDoc( bsoncxx::document::view doc )
    {
      using util::bsonValue;
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      auto d = document{};
      if ( auto set = util::bsonValueIfExists<bsoncxx::document::view>( "$set", doc ); set ) return bsoncxx::document::value{ doc };

      d << "$set" << open_document;

      for ( const auto& e : doc )
      {
        if ( e.key() == "_id" ) continue;
        if ( e.key() == "$unset" ) continue;

        switch ( e.type() )
        {
        case bsoncxx::type::k_array: d << e.key() << e.get_array();
          break;
        case bsoncxx::type::k_bool: d << e.key() << e.get_bool();
          break;
        case bsoncxx::type::k_date: d << e.key() << e.get_date();
          break;
        case bsoncxx::type::k_decimal128: d << e.key() << e.get_decimal128();
          break;
        case bsoncxx::type::k_document: d << e.key() << e.get_document();
          break;
        case bsoncxx::type::k_double: d << e.key() << e.get_double();
          break;
        case bsoncxx::type::k_int32: d << e.key() << e.get_int32();
          break;
        case bsoncxx::type::k_int64: d << e.key() << e.get_int64();
          break;
        case bsoncxx::type::k_oid: d << e.key() << e.get_oid();
          break;
        case bsoncxx::type::k_null: d << e.key() << e.get_null();
          break;
        case bsoncxx::type::k_timestamp: d << e.key() << e.get_timestamp();
          break;
        case bsoncxx::type::k_string:
          d << e.key() << bsonValue<std::string>( e.key(), doc );
          break;
        default:
          LOG_WARN << "Un-mapped bson type: " << bsoncxx::to_string( e.type() );
          d << e.key() << bsoncxx::types::b_null{};
        }
      }

      d << close_document;

      if ( auto unset = util::bsonValueIfExists<bsoncxx::document::view>( "$unset", doc ); unset ) d << "$unset" << *unset;

      return d << finalize;
    }

    mongocxx::options::update updateOptions( const model::Document& document )
    {
      using util::bsonValueIfExists;

      const auto options = document.options();
      auto opts = mongocxx::options::update{};

      if ( options )
      {
        if ( auto validate = bsonValueIfExists<bool>( "bypassValidation", *options ); validate ) opts.bypass_document_validation( *validate );
        if ( auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *options ); col ) opts.collation( *col );
        if ( auto upsert = bsonValueIfExists<bool>( "upsert", *options ); upsert ) opts.upsert( *upsert );
        if ( auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options ); wc ) opts.write_concern( internal::writeConcern( *wc ) );
        if ( auto af = bsonValueIfExists<bsoncxx::array::view>( "arrayFilters", *options ); af ) opts.array_filters( *af );
      }

      return opts;
    }

    awaitable<bsoncxx::document::view_or_value> updateOne( const model::Document& model )
    {
      using util::bsonValue;
      using util::bsonValueIfExists;

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
      if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

      const auto vhd = [&]( std::string_view action = "update" ) -> awaitable<bsoncxx::document::view_or_value>
      {
        if ( skip && *skip ) co_return document{} << "skipVersion" << true << finalize;

        const auto updated = ( *client )[dbname][collname].find_one( document{} << "_id" << oid << finalize );
        if ( !updated ) co_return model::notFound();
        if ( bsonValueIfExists<std::string>( "error", *updated ) )
        {
          co_return bsoncxx::document::value{ updated.value() };
        }

        auto d = co_await history( document{} <<
          "action" << action <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << updated->view() <<
          finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", d ) ) co_return d;

        co_return bsoncxx::document::view_or_value{ document{} << "document" << updated->view() << "history" << d << finalize };
      };

      const auto result = ( *client )[dbname][collname].update_one(
          document{} << "_id" << oid << finalize, updateDoc( doc ), opts );
      if ( opts.write_concern()->is_acknowledged() )
      {
        if ( result )
        {
          if ( result->upserted_count() > 0 )
          {
            LOG_INFO << "Upserted document " << dbname << ':' << collname << ':' << oid.to_string();
            co_return co_await vhd( "create" );
          }
          LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
          co_return co_await vhd();
        }
        LOG_WARN << "Unable to update document " << dbname << ':' << collname << ':' << oid.to_string();
      }
      else
      {
        LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
        co_return co_await vhd();
      }

      co_return model::updateError();
    }

    awaitable<bsoncxx::document::view_or_value> updateOneByFilter( const model::Document& model, bsoncxx::oid oid )
    {
      using util::bsonValue;
      using util::bsonValueIfExists;

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
      if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

      const auto vhd = [&]( std::string_view action = "update" ) -> awaitable<bsoncxx::document::view_or_value>
      {
        if ( skip && *skip ) co_return document{} << "skipVersion" << true << finalize;

        const auto updated = ( *client )[dbname][collname].find_one( filter );
        if ( !updated ) co_return model::notFound();
        if ( bsonValueIfExists<std::string>( "error", *updated ) )
        {
          co_return bsoncxx::document::value{ updated.value() };
        }

        auto vhd = co_await history( document{} <<
          "action" << action <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << updated->view() <<
          finalize, client, metadata );
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
          if ( result->upserted_count() > 0 )
          {
            LOG_INFO << "Upserted document " << dbname << ':' << collname << ':' << oid.to_string();
            co_return co_await vhd( "create" );
          }
          LOG_INFO << "Updated document " << dbname << ':' << collname << ':' << oid.to_string();
          co_return co_await vhd();
        }
        LOG_WARN << "Unable to update document " << dbname << ':' << collname
          << ':' << oid.to_string();
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
      using util::bsonValue;
      using util::bsonValueIfExists;

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
        if ( auto validate = bsonValueIfExists<bool>( "bypassValidation", *options ); validate ) opts.bypass_document_validation( *validate );
        if ( auto col = bsonValueIfExists<bsoncxx::document::view>( "collation", *options ); col ) opts.collation( *col );
        if ( auto upsert = bsonValueIfExists<bool>( "upsert", *options ); upsert ) opts.upsert( *upsert );
        if ( auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options ); wc ) opts.write_concern( internal::writeConcern( *wc ) );
      }

      const auto vhd = [&]( std::string_view action = "replace" ) -> awaitable<bsoncxx::document::view_or_value>
      {
        if ( skip && *skip )
        {
          co_return document{} << "skipVersion" << true << finalize;
        }

        if ( oid )
        {
          auto vhd = co_await history( document{} <<
            "action" << action <<
            "database" << dbname <<
            "collection" << collname <<
            "document" << replace <<
            finalize, client, metadata );
          if ( bsonValueIfExists<std::string>( "error", vhd ) ) co_return vhd;

          co_return document{} << "document" << replace << "history" << vhd << finalize;
        }

        const auto updated = ( *client )[dbname][collname].find_one( filter );
        if ( !updated )
        {
          LOG_WARN << "Updated document not found in " <<
            dbname << ':' << collname << " by filter " << bsoncxx::to_json( filter );
          co_return model::notFound();
        }

        auto d = co_await history( document{} <<
          "action" << "replace" <<
          "database" << dbname <<
          "collection" << collname <<
          "document" << updated->view() <<
          finalize, client, metadata );
        if ( bsonValueIfExists<std::string>( "error", d ) ) co_return d;

        co_return document{} << "document" << updated->view() << "history" << d << finalize;
      };

      if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );
      const auto result = ( *client )[dbname][collname].replace_one( filter, replace, opts );
      if ( opts.write_concern()->is_acknowledged() )
      {
        if ( result )
        {
          LOG_INFO << "Updated document in " << dbname << ':' << collname << " with filter " << bsoncxx::to_json( filter );
          co_return co_await vhd();
        }
        LOG_INFO << "Unable to update document in " << dbname << ':' << collname <<
          " with filter " << bsoncxx::to_json( filter );
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
      using util::bsonValue;
      using util::bsonValueIfExists;

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

      if ( const auto replace = bsonValueIfExists<bsoncxx::document::view>( "replace", doc ); replace ) co_return co_await replaceOne( model );

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

      auto ids = std::vector< bsoncxx::oid >{};
      if ( !( skip && *skip ) )
      {
        ids.reserve( 64 );
        auto results = ( *client )[dbname][collname].find( *filter );
        for ( const auto& d : results ) ids.push_back( bsonValue<bsoncxx::oid>( "_id", d ) );
      }

      const auto result = ( *client )[dbname][collname].update_many( *filter, updateDoc( *update ), opts );

      const auto vhd = [&]() -> awaitable<bsoncxx::document::view_or_value>
      {
        if ( skip && *skip ) co_return document{} << "skipVersion" << true << finalize;

        auto success = bsoncxx::builder::basic::array{};
        auto fail = bsoncxx::builder::basic::array{};
        auto vh = bsoncxx::builder::basic::array{};

        for ( const auto id : ids )
        {
          const auto res = ( *client )[dbname][collname].find_one( document{} << "_id" << id << finalize );
          if ( !res ) continue;
          auto vhd = co_await history( document{} << "action" << "update" <<
            "database" << dbname << "collection" << collname <<
            "document" << res->view() << finalize,
            client, metadata );
          if ( bsonValueIfExists<std::string>( "error", vhd ) ) fail.append( id );
          else
          {
            success.append( id );
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
          LOG_INFO << "Updated " << result->modified_count() << " documents in " << dbname << ':' << collname;
          co_return co_await vhd();
        }
        LOG_WARN << "Unable to update documents in " << dbname << ':' << collname;
      }
      else
      {
        LOG_INFO << "Updated documents in " << dbname << ':' << collname;
        co_return co_await vhd();
      }

      co_return model::updateError();
    }

    awaitable<bsoncxx::document::view_or_value> remove( const model::Document& model )
    {
      using util::bsonValue;
      using util::bsonValueIfExists;

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
        if ( auto wc = bsonValueIfExists<bsoncxx::document::view>( "writeConcern", *options ); wc ) opts.write_concern( internal::writeConcern( *wc ) );
        if ( auto co = bsonValueIfExists<bsoncxx::document::view>( "collation", *options ); co ) opts.collation( *co );
        if ( auto co = bsonValueIfExists<bsoncxx::document::view>( "hint", *options ); co ) opts.hint( { *co } );
        if ( auto co = bsonValueIfExists<bsoncxx::document::view>( "let", *options ); co ) opts.let( *co );
      }

      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        co_return model::poolExhausted();
      }

      auto& client = *cliento;
      if ( !opts.write_concern() ) opts.write_concern( client->write_concern() );

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

          auto vdoc = document{};
          vdoc <<
            "action" << "delete" <<
            "database" << dbname <<
            "collection" << collname <<
            "document" << d;

          if ( metadata ) vdoc << "metadata" << *metadata;

          auto vhdr = co_await history( vdoc << finalize, client, metadata );
          vh.append( vhdr );
          co_return true;
        };

        const auto oid = bsonValue<bsoncxx::oid>( "_id", d );
        const auto res = ( *client )[dbname][collname].delete_one( document{} << "_id" << oid << finalize, opts );

        if ( opts.write_concern()->is_acknowledged() )
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

      for ( const auto& d : docs.view() )
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
      using util::bsonValueIfExists;

      const auto doc = model.document();
      const auto dbname = model.database();
      const auto collname = model.collection();
      const auto metadata = model.metadata();
      const auto skip = model.skipVersion();

      const auto insert = bsonValueIfExists<bsoncxx::array::view>( "insert", doc );
      auto icount = 0;
      auto ihcount = 0;
      const auto rem = bsonValueIfExists<bsoncxx::array::view>( "remove", doc );
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
        for ( const auto& e : *insert )
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

          for ( const auto& e : *insert )
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
        for ( const auto& e : *rem )
        {
          bw.append( mongocxx::model::delete_one{ e.get_document().view() } );
          ++rcount;
        }

        if ( !skip || !*skip )
        {
          if ( histv.empty() ) histv.reserve( rcount );

          for ( const auto& e : *rem )
          {
            auto res = ( *client )[dbname][collname].find( e.get_document().view() );
            for ( const auto& d : res )
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
          "remove" << r->deleted_count() << finalize;
      }

      co_return document{} <<
        "create" << icount <<
        "history" << ihcount <<
        "remove" << rcount <<
        finalize;
    }

    awaitable<bsoncxx::document::view_or_value> pipeline( const model::Document& model )
    {
      using util::bsonValueIfExists;
      using bsoncxx::builder::stream::array;
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;

      LOG_DEBUG << "Executing aggregation pipeline query";
      const auto doc = model.document();
      const auto dbname = model.database();
      const auto collname = model.collection();

      const auto spec = bsonValueIfExists<bsoncxx::array::view>( "specification", doc );

      if ( !spec )
      {
        LOG_WARN << "No aggregation specification";
        co_return model::withMessage( "No aggregation specification." );
      }

      auto pipeline = mongocxx::pipeline{};
      for ( const auto& s : *spec ) pipeline.append_stage( s.get_document().view() );

      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        co_return model::poolExhausted();
      }

      const auto& client = *cliento;
      auto aggregate = ( *client )[dbname][collname].aggregate( pipeline );

      auto arr = array{};
      for ( auto&& d : aggregate ) arr << d;
      co_return document{} << "results" << ( arr << finalize ) << finalize;
    }

    boost::asio::awaitable<bsoncxx::document::view_or_value> processCollection( std::string_view action, const model::Document& document )
    {
      using std::operator""sv;
      if ( action == "createTimeseries"sv ) co_return co_await createTimeseries( document );
      if ( action == "dropCollection"sv ) co_return co_await internal::dropCollection( document );
      if ( action == "createCollection"sv ) co_return co_await internal::createCollection( document );
      if ( action == "renameCollection"sv ) co_return co_await internal::renameCollection( document );

      LOG_INFO << "Invalid action " << action << " in document " << document.json();
      co_return model::invalidAction();
    }

    boost::asio::awaitable<bsoncxx::document::view_or_value> processRemaining( std::string_view action, const model::Document& document )
    {
      using std::operator""sv;
      if ( action == "dropIndex"sv ) co_return co_await dropIndex( document );
      if ( action == "bulk"sv ) co_return co_await bulk( document );
      if ( action == "pipeline"sv ) co_return co_await pipeline( document );
      if ( action == "transaction"sv ) co_return co_await internal::transaction( document );
      if ( action == "distinct"sv ) co_return co_await distinct( document );
      co_return co_await processCollection( action, document ); // hack to get around GCC issue with number of ifs
    }

    boost::asio::awaitable<bsoncxx::document::view_or_value> process( const model::Document& document )
    {
      using std::operator""sv;
      try
      {
        const auto action = document.action();

        if ( action == "create"sv ) co_return co_await create( document );
        if ( action == "update"sv ) co_return co_await update( document );
        if ( action == "retrieve"sv ) co_return co_await retrieve( document );
        if ( action == "delete"sv ) co_return co_await remove( document );
        if ( action == "count"sv ) co_return co_await count( document );
        if ( action == "index"sv ) co_return co_await index( document );
        co_return co_await processRemaining( action, document ); // hack to get around GCC issue with number of ifs
      }
      catch ( const mongocxx::bulk_write_exception& be )
      {
        LOG_CRIT << "Error processing database action " << document.action() <<
         " code: " << be.code().message() << ", message: " << be.what();
        LOG_INFO << document.json();

        co_return model::withMessage( std::format( "Error processing database action {}", document.action() ) );
      }
      catch ( const mongocxx::logic_error& le )
      {
        LOG_CRIT << "Error processing database action " << document.action() <<
         " code: " << le.code().message() << ", message: " << le.what();
        LOG_INFO << document.json();

        co_return model::withMessage( std::format( "Error processing database action {}", document.action() ) );
      }
      catch ( const mongocxx::operation_exception& oe )
      {
        LOG_CRIT << "Error processing database action " << document.action() <<
          " code: " << oe.code().message() << ", message: " << oe.what();
        LOG_INFO << document.json();

        co_return model::withMessage( std::format( "Error processing database action {}", document.action() ) );
      }
      catch ( const std::exception& ex )
      {
        LOG_CRIT << "Error processing database action " << document.action() << ". " << ex.what();
        LOG_INFO << document.json();
        co_return model::unexpectedError();
      }
    }
  }
}

boost::asio::awaitable<bsoncxx::document::view_or_value> spt::db::process( const model::Document& document )
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
  if ( doc.find( "_id" ) != doc.end() && bsoncxx::type::k_oid == doc["_id"].type() )
  {
    metric.id = util::bsonValue<bsoncxx::oid>( "_id", doc );
  }

  metric.application = document.application();
  metric.correlationId = document.correlationId();
  metric.message = bsonValueIfExists<std::string>( "error", value.view() );
  metric.size = value.view().length();

  auto skip = document.skipMetric();
  if ( skip && *skip ) LOG_INFO << "Skipping metric " << document.json();
  else MetricsCollector::instance().add( std::move( metric ) );

  co_return value;
}
