//
// Created by Rakesh on 23/09/2020.
//
#include "catch.hpp"
#include "../../src/util/bson.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <iostream>
#include <vector>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::bulk
{
  const auto oid1 = bsoncxx::oid{};
  const auto oid2 = bsoncxx::oid{};
  std::vector<bsoncxx::oid> oids;
  int64_t count = 0;
}

SCENARIO( "Bulk operation test suite", "[bulk]" )
{
  boost::asio::io_context ioc;

  GIVEN( "Connected to Mongo Service" )
  {
    tcp::socket s( ioc );
    tcp::resolver resolver( ioc );
    boost::asio::connect( s, resolver.resolve( "localhost", "2020" ));

    WHEN( "Creating documents in bulk" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };

      auto doc = document{} <<
          "action" << "bulk" <<
          "database" << "itest" <<
          "collection" << "test" <<
          "document" <<
            open_document <<
              "insert" <<
                open_array <<
                  open_document <<
                    "_id" << spt::itest::bulk::oid1 <<
                    "key" << "value1" <<
                  close_document <<
                  open_document <<
                    "_id" << spt::itest::bulk::oid2 <<
                    "key" << "value2" <<
                  close_document <<
                close_array <<
            close_document <<
          finalize;
      os.write( reinterpret_cast<const char*>( doc.view().data() ), doc.view().length() );
      std::cout << bsoncxx::to_json( doc.view() ) << '\n';

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "create" ) != option->end() );
      REQUIRE( option->find( "delete" ) != option->end() );
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
      spt::itest::bulk::count = *count;
    }

    AND_THEN( "Deleting documents in bulk" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };

      auto doc = document{} <<
        "action" << "bulk" <<
        "database" << "itest" <<
        "collection" << "test" <<
        "document" <<
          open_document <<
            "delete" <<
              open_array <<
                open_document << "_id" << spt::itest::bulk::oid1 << close_document <<
                open_document << "_id" << spt::itest::bulk::oid2 << close_document <<
              close_array <<
          close_document <<
        finalize;
      os.write( reinterpret_cast<const char*>( doc.view().data() ), doc.view().length() );

      const auto isize = s.send( buffer.data());
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate(
          reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value());
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "create" ) != option->end() );
      REQUIRE( option->find( "delete" ) != option->end() );
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
      REQUIRE( spt::itest::bulk::count > *count );
    }

    AND_THEN( "Creating a large batch of documents" )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      const auto size = 1000;
      spt::itest::bulk::oids.reserve( size );
      auto arr = basic::array{};
      for ( auto i = 0; i < size; ++i )
      {
        spt::itest::bulk::oids.emplace_back( bsoncxx::oid{} );
        arr.append(
            document{} <<
              "_id" << spt::itest::bulk::oids.back() <<
              "iter" << i <<
              "key1" << "value1" <<
              "key2" << "value2" <<
              "key3" << "value3" <<
              "key4" << "value4" <<
              "key5" << "value5" <<
              "sub1" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
              "sub2" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
              "sub3" <<
                open_document <<
                  "key1" << "value1" <<
                  "key2" << "value2" <<
                  "key3" << "value3" <<
                  "key4" << "value4" <<
                  "key5" << "value5" <<
                close_document <<
            finalize
            );
      }

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "bulk" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "insert", arr.extract() ) ) ) );
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
      REQUIRE( option->find( "create" ) != option->end() );
      REQUIRE( option->find( "delete" ) != option->end() );
    }

    AND_THEN( "Deleting a large batch of documents" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      auto arr = basic::array{};
      for ( auto& id : spt::itest::bulk::oids )
      {
        arr.append( basic::make_document( kvp( "_id", id ) ) );
      }

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "bulk" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "delete", arr.view() ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      std::cout << bsoncxx::to_json( *option ) << '\n';
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "create" ) != option->end() );
      REQUIRE( option->find( "delete" ) != option->end() );
    }

    s.close();
  }
}

