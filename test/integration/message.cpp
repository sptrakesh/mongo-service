//
// Created by Rakesh on 19/07/2020.
//

#include "catch.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <bsoncxx/validate.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;

SCENARIO( "Message payload test suite", "[message]" )
{
  boost::asio::io_context ioc;

  GIVEN( "Connected to Mongo Service" )
  {
    tcp::socket s( ioc );
    tcp::resolver resolver( ioc );
    boost::asio::connect( s, resolver.resolve( "localhost", "2020" ) );

    WHEN( "Sending non-bson payload" )
    {
      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      os << "hello world";

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      const auto data = buffer.data();
      std::string resp{ boost::asio::buffers_begin( data ), boost::asio::buffers_end( data ) };
      std::cout << "[message] " << resp << '\n';
      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      REQUIRE( option->find( "error" ) != option->end() );
    }

    WHEN( "Sending random bson payload" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document( kvp( "hello", "world" ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      const auto data = buffer.data();
      std::string resp{ boost::asio::buffers_begin( data ), boost::asio::buffers_end( data ) };
      std::cout << "[message] " << resp << '\n';
      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      REQUIRE( option->find( "error" ) != option->end() );
      REQUIRE( option->find( "fields" ) != option->end() );
    }

    WHEN( "Sending bson payload without document._id" )
    {
      namespace basic = bsoncxx::builder::basic;
      using basic::kvp;

      boost::asio::streambuf buffer;
      std::ostream os{ &buffer };
      bsoncxx::document::value document = basic::make_document(
          kvp( "action", "create" ),
          kvp( "database", "itest" ),
          kvp( "collection", "test" ),
          kvp( "document", basic::make_document( kvp( "key", "value" ) ) ) );
      os.write( reinterpret_cast<const char*>( document.view().data() ), document.view().length() );

      const auto isize = s.send( buffer.data() );
      buffer.consume( isize );

      const auto osize = s.receive( buffer.prepare( 128 * 1024 ) );
      buffer.commit( osize );

      const auto data = buffer.data();
      std::string resp{ boost::asio::buffers_begin( data ), boost::asio::buffers_end( data ) };
      std::cout << "[message] " << resp << '\n';
      REQUIRE( isize != osize );

      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), osize );
      REQUIRE( option.has_value() );
      REQUIRE( option->find( "error" ) != option->end() );
    }

    s.close();
  }
}

