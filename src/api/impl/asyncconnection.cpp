//
// Created by Rakesh on 22/02/2022.
//

#include "asyncconnection.h"
#include "settings.h"
#if defined __has_include
  #if __has_include("../../log/NanoLog.h")
    #include "../../log/NanoLog.h"
  #else
    #include <log/NanoLog.h>
  #endif
#endif

#include <boost/asio/connect.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>

using spt::mongoservice::api::impl::AsyncConnection;

AsyncConnection::AsyncConnection( boost::asio::io_context& ioc, std::string_view h,
    std::string_view p ) :
    s{ ioc }, resolver( ioc ),
    host{ h.data(), h.size() }, port{ p.data(), p.size() }
{
  boost::system::error_code ec;
  endpoints = resolver.resolve( h, p, ec );
  if ( ec )
  {
    LOG_CRIT << "Error resolving service " << host << ':' << port << ". " << ec.message();
    throw std::runtime_error{ "Error resolving service host:port" };
  }
}

AsyncConnection::~AsyncConnection()
{
  boost::system::error_code ec;
  s.close( ec );
  if ( ec ) LOG_WARN << "Error closing socket " << ec.message();
}

auto AsyncConnection::execute( bsoncxx::document::view view ) -> boost::asio::awaitable<Response>
{
  static constexpr int bufSize = 1024;

  try
  {
    boost::system::error_code ec;
    if ( !s.is_open() )
    {
      LOG_INFO << "Connecting to service " << host << ':' << port << ". " << bsoncxx::to_json(view);
      co_await boost::asio::async_connect( s, endpoints,
          boost::asio::redirect_error( boost::asio::use_awaitable, ec ) );
      if ( ec )
      {
        LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
        co_return std::nullopt;
      }
    }

    auto isize = co_await boost::asio::async_write(
        s, boost::asio::buffer( view.data(), view.length() ),
        boost::asio::redirect_error( boost::asio::use_awaitable, ec ) );
    if ( ec )
    {
      LOG_WARN << "Error writing request to service " << ec.message();
      co_return std::nullopt;
    }
    LOG_DEBUG << "Wrote " << int(isize) << " bytes to socket";

    uint8_t data[bufSize];

    const auto documentSize = [&data]( std::size_t length )
    {
      if ( length < 5 ) return length;

      const auto d = reinterpret_cast<const uint8_t*>( data );
      uint32_t len;
      memcpy( &len, d, sizeof(len) );
      return std::size_t( len );
    };

    std::size_t osize = co_await s.async_read_some( boost::asio::buffer( data ),
        boost::asio::redirect_error( boost::asio::use_awaitable, ec ) );
    if ( ec )
    {
      LOG_WARN << "Error reading response from service " << ec.message();
      co_return std::nullopt;
    }
    const auto docSize = documentSize( osize );

    if ( docSize <= bufSize )
    {
      const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( data ), docSize );
      if ( !option )
      {
        LOG_WARN << "Invalid BSON data in response to " << bsoncxx::to_json( view );
        co_return std::nullopt;
      }
      co_return bsoncxx::document::value{ *option };
    }

    LOG_DEBUG << "Read " << int(osize) << " bytes of " << int(docSize) << " total";
    auto read = osize;
    std::vector<uint8_t> rbuf;
    rbuf.reserve( docSize );
    rbuf.insert( rbuf.end(), data, data + osize );

    while ( read != docSize ) // flawfinder: ignore
    {
      osize = co_await s.async_read_some( boost::asio::buffer( data ),
          boost::asio::redirect_error( boost::asio::use_awaitable, ec ) );
      if ( ec )
      {
        LOG_WARN << "Error reading response from service " << ec.message();
        co_return std::nullopt;
      }
      rbuf.insert( rbuf.end(), data, data + osize );
      read += osize;
      LOG_DEBUG << "Read " << int(read) << " bytes of " << int(docSize) << " total"; // flawfinder: ignore
    }

    const auto option = bsoncxx::validate( reinterpret_cast<const uint8_t*>( rbuf.data() ), docSize );
    if ( !option )
    {
      LOG_WARN << "Invalid BSON data in response to " << bsoncxx::to_json( view );
      co_return std::nullopt;
    }
    co_return bsoncxx::document::value{ *option };
  }
  catch ( std::exception& ex )
  {
    LOG_CRIT << "Exception executing request " << ex.what();
    co_return std::nullopt;
  }
}

auto spt::mongoservice::api::impl::createAsyncConnection() -> std::unique_ptr<AsyncConnection>
{
  auto& settings = ApiSettings::instance();
  return std::make_unique<AsyncConnection>( *settings.ioc, settings.server, settings.port );
}
