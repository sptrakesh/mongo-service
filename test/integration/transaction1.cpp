//
// Created by Rakesh on 17/07/2021.
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
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using tcp = boost::asio::ip::tcp;

namespace spt::itest::trans1
{
  const auto oid1 = bsoncxx::oid{};
  const auto oid2 = bsoncxx::oid{};
  std::string vhdb;
  std::string vhc;
}

SCENARIO( "Transaction test suite1", "[transaction1]" )
{
  boost::asio::io_context ioc;

  GIVEN( "Connected to Mongo Service" )
  {
    tcp::socket s( ioc );
    tcp::resolver resolver( ioc );
    boost::asio::connect( s, resolver.resolve( "localhost", "2020" ) );

    WHEN( "Executing a transaction" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "transaction" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document(
              kvp( "items", basic::make_array(
                  basic::make_document(
                      kvp( "action", "create" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document(
                          kvp( "key", "value1" ), kvp( "_id", spt::itest::trans1::oid1 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "create" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document(
                          kvp( "key", "value2" ), kvp( "_id", spt::itest::trans1::oid2 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "delete" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document( kvp( "_id", spt::itest::trans1::oid1 ) ) )
                  ),
                  basic::make_document(
                      kvp( "action", "delete" ),
                      kvp( "database", "itest" ),
                      kvp( "collection", "test" ),
                      kvp( "document", basic::make_document( kvp( "_id", spt::itest::trans1::oid2 ) ) )
                  )
                )
             )
          ))
      );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( document.view() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "created" ) != option->end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "created", *option ) == 2 );
      REQUIRE( option->find( "updated" ) != option->end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "updated", *option ) == 0 );
      REQUIRE( option->find( "deleted" ) != option->end() );
      REQUIRE( spt::util::bsonValue<int32_t>( "deleted", *option ) == 2 );

      const auto h = spt::util::bsonValueIfExists<bsoncxx::document::view>( "history", *option );
      REQUIRE( h );

      const auto vd = spt::util::bsonValueIfExists<std::string>( "database", *h );
      REQUIRE( vd );
      spt::itest::trans1::vhdb = *vd;

      const auto vc = spt::util::bsonValueIfExists<std::string>( "collection", *h );
      REQUIRE( vc );
      spt::itest::trans1::vhc = *vc;

      const auto c = spt::util::bsonValueIfExists<bsoncxx::array::view>( "created", *h );
      const auto d = spt::util::bsonValueIfExists<bsoncxx::array::view>( "deleted", *h );

      auto num = 0;
      for ( auto&& i : *c )
      {
        REQUIRE( i.type() == bsoncxx::type::k_oid );
        ++num;
      }
      REQUIRE( num == 2 );

      num = 0;
      for ( auto&& i : *d )
      {
        REQUIRE( i.type() == bsoncxx::type::k_oid );
        ++num;
      }
      REQUIRE( num == 2 );
    }

    AND_THEN( "Retrieve the version history for first document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::trans1::vhdb ),
          kvp( "collection", spt::itest::trans1::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::trans1::oid1 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) == option->end() );
      REQUIRE( option->find( "results" ) != option->end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", *option );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        REQUIRE( e.type() == bsoncxx::type::k_document );
        ++i;
      }
      REQUIRE( i == 2 );
    }

    AND_THEN( "Retrieve the version history for second document" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "retrieve" ),
          kvp( "database", spt::itest::trans1::vhdb ),
          kvp( "collection", spt::itest::trans1::vhc ),
          kvp( "document", basic::make_document( kvp( "entity._id", spt::itest::trans1::oid2 ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      LOG_INFO << "[trans1] " << bsoncxx::to_json( *option );
      REQUIRE( option->find( "error" ) == option->end() );
      REQUIRE( option->find( "result" ) == option->end() );
      REQUIRE( option->find( "results" ) != option->end() );

      const auto arr = spt::util::bsonValue<bsoncxx::array::view>( "results", *option );
      REQUIRE_FALSE( arr.empty() );

      auto i = 0;
      for ( auto e : arr )
      {
        REQUIRE( e.type() == bsoncxx::type::k_document );
        ++i;
      }
      REQUIRE( i == 2 );
    }

    s.close();
  }
}
