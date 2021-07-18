//
// Created by Rakesh on 28/07/2020.
//
#include "catch.hpp"
#include "../../src/log/NanoLog.h"
#include "../../src/util/bson.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using tcp = boost::asio::ip::tcp;

SCENARIO( "Index test suite", "[index]" )
{
  boost::asio::io_context ioc;

  GIVEN( "Connected to Mongo Service" )
  {
    tcp::socket s( ioc );
    tcp::resolver resolver( ioc );
    boost::asio::connect( s, resolver.resolve( "localhost", "2020" ) );

    WHEN( "Creating an index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused", 1 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", *option );
      REQUIRE( name );
    }

    AND_THEN( "Creating the index again" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused", 1 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", *option );
      REQUIRE( name );
    }

    AND_THEN( "Dropping the index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "dropIndex" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "specification", basic::make_document( kvp( "unused", 1 ) ) ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );
    }

    WHEN( "Creating a unique index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused1", 1 ) ) ),
          kvp( "options", basic::make_document(
              kvp( "name", "uniqueIndex" ),
              kvp( "unique", true ),
              kvp( "expireAfterSeconds", 5 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", *option );
      REQUIRE( name );
    }

    AND_THEN( "Creating a unique index again" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "index" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "unused1", 1 ) ) ),
          kvp( "options", basic::make_document(
              kvp( "name", "uniqueIndex" ),
              kvp( "unique", true ),
              kvp( "expireAfterSeconds", 5 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );

      const auto name = spt::util::bsonValueIfExists<std::string>( "name", *option );
      REQUIRE( name );
    }

    AND_THEN( "Dropping the unique index" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "dropIndex" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "name", "uniqueIndex" ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[index] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );
    }
  }
}
