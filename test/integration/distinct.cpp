//
// Created by Rakesh on 07/12/2024.
//
#include "../../src/api/api.hpp"
#include "../../src/common/util/bson.hpp"

#include <catch2/catch_test_macros.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace
{
  namespace pdistinct
  {
    struct Fixture
    {
      ~Fixture()
      {
        remove( oid1 );
        remove( oid2 );
        remove( oid3 );
        remove( oid4 );
      }

      void remove( bsoncxx::oid oid )
      {
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        const auto request  = spt::mongoservice::api::Request::_delete(
            "itest", "test",
            document{} << "_id" << oid << finalize );
        spt::mongoservice::api::execute( request );
      }

      bsoncxx::oid oid1;
      bsoncxx::oid oid2;
      bsoncxx::oid oid3;
      bsoncxx::oid oid4;
    };
  }
}

TEST_CASE_PERSISTENT_FIXTURE( pdistinct::Fixture, "Distinct test suite", "[distinct]" )
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  GIVEN( "Connected to Mongo Service" )
  {
    WHEN( "Creating first document" )
    {
      const auto request  = spt::mongoservice::api::Request::create(
          "itest", "test",
          document{} << "string" << "value1" << "_id" << oid1 << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
    }

    THEN( "Retrieving distinct values" )
    {
      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} << "field" << "string" << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 1 );
    }

    AND_WHEN( "Creating second document" )
    {
      const auto request  = spt::mongoservice::api::Request::create(
          "itest", "test",
          document{} << "string" << "value2" << "_id" << oid2 << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
    }

    AND_THEN( "Retrieving distinct values" )
    {
      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} << "field" << "string" << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 2 );
    }

    AND_WHEN( "Creating third document" )
    {
      const auto request  = spt::mongoservice::api::Request::create(
          "itest", "test",
          document{} << "string" << "value2" << "_id" << oid3 << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
    }

    AND_THEN( "Retrieving distinct values" )
    {
      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} << "field" << "string" << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 2 );
    }

    AND_WHEN( "Creating fourth document" )
    {
      const auto request  = spt::mongoservice::api::Request::create(
          "itest", "test",
          document{} << "string" << "value" << "_id" << oid4 << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      const auto opt = option->view();
      REQUIRE( opt.find( "error" ) == opt.end() );
      REQUIRE( opt.find( "_id" ) != opt.end() );
    }

    AND_THEN( "Retrieving distinct values" )
    {
      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} << "field" << "string" << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 3 );
    }

    AND_THEN( "Retrieving distinct values with filter" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} <<
            "field" << "string" <<
            "filter" <<
              open_document <<
                "_id" << open_document << "$ne" << oid4 << close_document <<
              close_document <<
          finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 2 );
    }

    AND_THEN( "Retrieving distinct values for missing field" )
    {
      const auto request  = spt::mongoservice::api::Request::distinct(
          "itest", "test",
          document{} << "field" << "invalid" << finalize );

      const auto [type, option] = spt::mongoservice::api::execute( request );
      REQUIRE( type == spt::mongoservice::api::ResultType::success );
      REQUIRE( option.has_value() );
      LOG_INFO << "[distinct] " << bsoncxx::to_json( *option );
      const auto opt = option->view();
      CHECK( opt.find( "error" ) == opt.end() );
      CHECK( opt.find( "result" ) == opt.end() );
      REQUIRE( opt.find( "results" ) != opt.end() );

      const auto results = opt["results"].get_array().value;
      REQUIRE( results.begin()->type() == bsoncxx::type::k_document );
      const auto doc = results.begin()->get_document().value;
      REQUIRE( doc.find( "values" ) != doc.end() );
      REQUIRE( doc["values"].type() == bsoncxx::type::k_array );
      CHECK( std::distance( doc["values"].get_array().value.begin(), doc["values"].get_array().value.end() ) == 0 );
    }
  }
}
