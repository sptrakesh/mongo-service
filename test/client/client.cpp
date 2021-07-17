//
// Created by Rakesh on 15/07/2021.
//

#include "client.h"
#include "context.h"
#include "../../src/log/NanoLog.h"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>

#include <iostream>
#include <vector>

using boost::asio::use_awaitable;

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

using spt::client::Client;

Client::Client( boost::asio::io_context& ioc, std::string_view h,
  std::string_view p, boost::system::error_code& ec ) :
    s{ ioc }, resolver( ioc ),
    host{ h.data(), h.size() }, port{ p.data(), p.size() }
{
  resolver.resolve( host, port, ec );
  if ( ec ) LOG_WARN << "Error resolving service " << ec.message();
}

Client::~Client()
{
  boost::system::error_code ec;
  s.close( ec );

  if ( ec ) LOG_WARN << "Error closing socket " << ec.message();
}

boost::asio::awaitable<std::optional<bsoncxx::document::value>> Client::execute(
    bsoncxx::document::view view )
{
  static constexpr int bufSize = 1024;

  try
  {
    if ( !s.is_open() ) co_await boost::asio::async_connect( s, resolver.resolve( host, port ), use_awaitable );
    auto isize = co_await boost::asio::async_write(
        s, boost::asio::buffer( view.data(), view.length() ), use_awaitable );
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

    std::size_t osize = co_await s.async_read_some( boost::asio::buffer( data ), use_awaitable );
    const auto docSize = documentSize( osize );
    LOG_DEBUG << "Read " << int(osize) << " bytes of " << int(docSize) << " total";

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

    auto read = osize;
    std::vector<uint8_t> rbuf;
    rbuf.reserve( docSize );
    rbuf.insert( rbuf.end(), data, data + osize );

    while ( read != docSize )
    {
      osize = co_await s.async_read_some( boost::asio::buffer( data ), use_awaitable );
      rbuf.insert( rbuf.end(), data, data + osize );
      read += osize;
      LOG_DEBUG << "Read " << int(read) << " bytes of " << int(docSize) << " total";
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
    std::cerr << "Exception executing request " << ex.what() << std::endl;
    LOG_CRIT << "Exception executing request " << ex.what();
    co_return std::nullopt;
  }
}

auto spt::client::createClient() -> std::unique_ptr<Client>
{
  boost::system::error_code ec;
  return std::make_unique<Client>( Context::instance().ioc, "localhost", "2020", ec );
  assert( !ec );
}
