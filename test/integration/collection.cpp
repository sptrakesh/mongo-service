//
// Created by Rakesh on 21/11/2024.
//

#include "../../src/api/api.hpp"
#include "../../src/common/util/bson.hpp"

#include <thread>
#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/stream/document.hpp>

using std::operator""s;

SCENARIO( "Collection test suite", "[collection]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    static const auto database = "itest"s;
    static const auto collection = "timeseries"s;

    WHEN( "A new collection is created" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      const auto request  = spt::mongoservice::api::Request::createCollection(
          database, collection,
          document{} <<
            "timeseries" <<
              open_document <<
                "timeField" << "timestamp" <<
                "metaField" << "data" <<
                "granularity" << "hours" <<
              close_document <<
          finalize );

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      const auto [type, option] = spt::mongoservice::api::execute( request, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[collection] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      REQUIRE( spt::util::bsonValue<std::string>( "database", opt ) == database );
      REQUIRE( spt::util::bsonValue<std::string>( "collection", opt ) == collection );
    }

    THEN( "Cannot create existing collection" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;

      const auto request  = spt::mongoservice::api::Request::createCollection(
          database, collection, document{} << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[collection] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) != opt.end() );
    }

    AND_THEN( "Document created in new collection with auto generated id" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      const auto request  = spt::mongoservice::api::Request::createTimeseries(
          database, collection,
          document{} <<
            "timestamp" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
            "data" <<
              open_document <<
                "property1" << "string" <<
                "property2" << 1234 <<
              close_document <<
            "value" << 345.668 <<
            finalize );

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      const auto [type, option] = spt::mongoservice::api::execute( request, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[collection] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      CHECK( opt.find( "_id" ) != opt.end() );
      CHECK( spt::util::bsonValue<std::string>( "database", opt ) == database );
      CHECK( spt::util::bsonValue<std::string>( "collection", opt ) == collection );
    }

    AND_THEN( "Document created in new collection with user specified id" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      const auto oid = bsoncxx::oid{};
      const auto request  = spt::mongoservice::api::Request::createTimeseries(
          database, collection,
          document{} <<
            "_id" << oid <<
            "timestamp" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
            "data" <<
              open_document <<
                "property1" << "string" <<
                "property2" << 1234 <<
              close_document <<
            "value" << 345.668 <<
            finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[collection] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "database" ) != opt.end() );
      REQUIRE( opt.find( "collection" ) != opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
      CHECK( spt::util::bsonValue<std::string>( "database", opt ) == database );
      CHECK( spt::util::bsonValue<std::string>( "collection", opt ) == collection );
      CHECK( spt::util::bsonValue<bsoncxx::oid>( "_id", opt ) == oid );
    }

    AND_THEN( "Drop collection" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::finalize;

      const auto request  = spt::mongoservice::api::Request::dropCollection( database, collection, document{} << finalize );

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      const auto [type, option] = spt::mongoservice::api::execute( request, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[collection] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "dropCollection" ) != opt.end() );
    }
  }
}
