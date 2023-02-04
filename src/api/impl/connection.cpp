//
// Created by Rakesh on 22/09/2020.
//

#include "connection.h"
#include "settings.h"
#if defined __has_include
  #if __has_include("../../log/NanoLog.h")
    #include "../../log/NanoLog.h"
  #else
    #include <log/NanoLog.h>
  #endif
#endif

#include <boost/asio/connect.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>

using spt::mongoservice::api::impl::Connection;

Connection::Connection( boost::asio::io_context& ioc, std::string_view h,
    std::string_view p ) : s{ ioc }, resolver{ ioc },
    host{ h.data(), h.size() }, port{ p.data(), p.size() }
{
  boost::system::error_code ec;
  endpoints = resolver.resolve( host, port, ec );
  if ( ec )
  {
    LOG_CRIT << "Error resolving service " << host << ':' << port << ". " << ec.message();
    throw std::runtime_error{ "Cannot resolve service host:port" };
  }

  boost::asio::connect( s, endpoints, ec );
  if ( ec )
  {
    LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
    throw std::runtime_error{ "Cannot connect to service host:port" };
  }
  boost::asio::socket_base::keep_alive option( true );
  s.set_option( option );
}

std::optional<bsoncxx::document::value> Connection::execute(
    bsoncxx::document::view document, std::size_t bufSize )
{
  std::ostream os{ &buffer };
  os.write( reinterpret_cast<const char*>( document.data() ), document.length() );

  boost::system::error_code ec;
  auto isize = socket().send( buffer.data(), 0, ec );
  if ( ec )
  {
    LOG_DEBUG << "Error sending data to socket " << ec.message();
    s.close( ec );
    isize = socket().send( buffer.data(), 0, ec );
  }

  buffer.consume( isize );

  const auto documentSize = [this]( std::size_t length )
  {
    if ( length < 5 ) return length;

    const auto data = reinterpret_cast<const uint8_t*>( buffer.data().data() );
    uint32_t len;
    memcpy( &len, data, sizeof(len) );
    return std::size_t( len );
  };

  auto osize = socket().receive( buffer.prepare( bufSize ), 0, ec );
  if ( ec )
  {
    LOG_DEBUG << "Error reading data from socket " << ec.message();
    s.close( ec );
    buffer.consume( buffer.size() );
    return std::nullopt;
  }

  buffer.commit( osize );
  std::size_t read = osize;

  const auto docSize = documentSize( osize );
  while ( read < docSize ) // flawfinder: ignore
  {
    osize = socket().receive( buffer.prepare( bufSize ) );
    buffer.commit( osize );
    read += osize;
  }

  const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), docSize );
  buffer.consume( buffer.size() );
  if ( option ) return bsoncxx::document::value{ option.value() };

  LOG_INFO << "Invalid BSON with size " << int(osize) << " in response to " << bsoncxx::to_json( document );
  return std::nullopt;
}

tcp::socket& Connection::socket()
{
  boost::system::error_code ec;

  if ( ! s.is_open() )
  {
    LOG_DEBUG << "Re-opening closed connection.";
    boost::asio::connect( s, endpoints, ec );
    if ( ec )
    {
      LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
      throw std::runtime_error{ "Cannot connect to service host:port" };
    }
    boost::asio::socket_base::keep_alive option( true );
    s.set_option( option );
  }

  if ( !v )
  {
    s.close( ec );
    boost::asio::connect( s, endpoints, ec );
    if ( ec )
    {
      LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
      throw std::runtime_error{ "Cannot connect to service host:port" };
    }
    boost::asio::socket_base::keep_alive option( true );
    s.set_option( option );
    v = true;
  }

  return s;
}

std::unique_ptr<Connection> spt::mongoservice::api::impl::create()
{
  auto& s = ApiSettings::instance();
  return std::make_unique<Connection>( *s.ioc, s.server, s.port );
}