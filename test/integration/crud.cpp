//
// Created by Rakesh on 22/07/2020.
//
#include "catch.hpp"
#include "../../src/util/bson.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::crud
{
  const auto oid = bsoncxx::oid{};
  std::string vhdb;
  std::string vhc;
  auto vhoid = bsoncxx::oid{};
  int64_t count = 0;
}

SCENARIO( "Simple CRUD test suite", "[crud]" )
{
  boost::asio::io_context ioc;

  GIVEN( "Connected to Mongo Service" )
  {
    tcp::socket s( ioc );
    tcp::resolver resolver( ioc );
    boost::asio::connect( s, resolver.resolve( "localhost", "2020" ) );

    WHEN( "Creating a document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "create" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "key", "value" ), kvp( "_id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "database" ) != option->end() );
      REQUIRE( option->find( "collection" ) != option->end() );
      REQUIRE( option->find( "entity" ) != option->end() );
      REQUIRE( option->find( "_id" ) != option->end() );
      REQUIRE( (*option)["entity"].get_oid().value == spt::itest::crud::oid );

      spt::itest::crud::vhdb = spt::util::bsonValue<std::string>( "database", *option );
      spt::itest::crud::vhc = spt::util::bsonValue<std::string>( "collection", *option );
      spt::itest::crud::vhoid = spt::util::bsonValue<bsoncxx::oid>( "_id", *option );
    }

    AND_THEN( "Retriving count of documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", *option );
      REQUIRE( count );
      spt::itest::crud::count = *count;
    }

    THEN( "Retrieving the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) != option->end() );
      REQUIRE( option->find( "results" ) == option->end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", *option );
      REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::oid );
      const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
      REQUIRE( key );
      REQUIRE( key == "value" );
    }

    THEN( "Retrieving the document by property" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "key", "value" ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) == option->end() );
      REQUIRE( option->find( "results" ) != option->end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", *option );
      REQUIRE_FALSE( arr.empty() );
      bool found = false;
      for ( auto e : arr )
      {
        const auto dv = e.get_document().view();
        if ( dv["_id"].get_oid().value == spt::itest::crud::oid ) found = true;
        const auto key = spt::util::bsonValueIfExists<std::string>( "key", dv );
        REQUIRE( key );
        REQUIRE( key == "value" );
      }
      REQUIRE( found );
    }

    AND_THEN( "Retrieve the version history document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::crud::vhdb ),
          kvp( "collection", spt::itest::crud::vhc ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::vhoid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) != option->end() );
      REQUIRE( option->find( "results" ) == option->end() );

      const auto dv = spt::util::bsonValue<bsoncxx::document::view>( "result", *option );
      REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::vhoid );

      const auto entity = spt::util::bsonValueIfExists<bsoncxx::document::view>( "entity", dv );
      REQUIRE( entity );
      REQUIRE( (*entity)["_id"].get_oid().value == spt::itest::crud::oid );
    }

    AND_THEN( "Retrieve the version history document by entity id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::crud::vhdb ),
          kvp( "collection", spt::itest::crud::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) == option->end() );
      REQUIRE( option->find( "results" ) != option->end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", *option );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        const auto dv = e.get_document().view();
        REQUIRE( dv["_id"].get_oid().value == spt::itest::crud::vhoid );

        const auto ev = spt::util::bsonValue<bsoncxx::document::view>( "entity", dv );
        REQUIRE( ev["_id"].get_oid().value == spt::itest::crud::oid );
        const auto key = spt::util::bsonValueIfExists<std::string>( "key", ev );
        REQUIRE( key );
        REQUIRE( key == "value" );
        ++i;
      }
      REQUIRE( i == 1 );
    }

    AND_THEN( "Updating the document by id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "update" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "key1", "value1" ), kvp( "_id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", *option );
      REQUIRE( doc );
      REQUIRE( doc->find( "_id" ) != doc->end() );
      REQUIRE( (*doc)["_id"].get_oid().value == spt::itest::crud::oid );

      auto key = spt::util::bsonValueIfExists<std::string>( "key", *doc );
      REQUIRE( key );
      REQUIRE( *key == "value" );

      key = spt::util::bsonValueIfExists<std::string>( "key1", *doc );
      REQUIRE( key );
      REQUIRE( *key == "value1" );
    }

    AND_THEN( "Updating the document without version history" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "update" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "skipVersion", true ),
          kvp( "document", basic::make_document(
              kvp( "key1", "value1" ), kvp( "_id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", *option );
      REQUIRE_FALSE( doc );

      const auto skip = spt::util::bsonValueIfExists<bool>( "skipVersion", *option );
      REQUIRE( skip );
      REQUIRE( *skip );
    }

    AND_THEN( "Deleting the document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "delete" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "_id", spt::itest::crud::oid ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "success" ) != option->end() );
      REQUIRE( option->find( "history" ) != option->end() );
    }

    AND_THEN( "Retriving count of documents after delete" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "count" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document() ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );

      const auto count = spt::util::bsonValueIfExists<int64_t>( "count", *option );
      REQUIRE( count );
      REQUIRE( spt::itest::crud::count > *count );
    }

    s.close();
  }
}
