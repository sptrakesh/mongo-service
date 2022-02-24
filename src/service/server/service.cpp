//
// Created by Rakesh on 18/07/2020.
//

#include "service.h"
#include "db/storage.h"
#include "model/configuration.h"
#include "model/document.h"
#include "model/errors.h"
#include "queue/queuemanager.h"
#include "queue/poller.h"
#include "../log/NanoLog.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include <vector>

using boost::asio::use_awaitable;

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace spt::server::coroutine
{
  boost::asio::awaitable<void> process( boost::asio::ip::tcp::socket& socket,
      const model::Document& doc )
  {
    if ( !doc.bson() )
    {
      auto view = spt::model::notBson();
      LOG_DEBUG << "Invalid bson received. Returning not bson message...";
      co_await boost::asio::async_write(
          socket, boost::asio::buffer( view.data(), view.length() ), use_awaitable );
    }
    else if ( !doc.valid() )
    {
      auto view = spt::model::missingField();
      LOG_DEBUG << "Invalid bson received.  Returning not bson message...";
      co_await boost::asio::async_write(
          socket, boost::asio::buffer( view.data(), view.length() ), use_awaitable );
    }
    else
    {
      try
      {
        auto v = co_await db::process( doc );
        auto view = v.view();
        co_await boost::asio::async_write(
            socket, boost::asio::buffer( view.data(), view.length() ), use_awaitable );
      }
      catch ( const std::exception& ex )
      {
        LOG_WARN << "Error processing request " << ex.what();
        auto view = model::unexpectedError();
        boost::system::error_code ec;
        boost::asio::write( socket, boost::asio::buffer( view.data(), view.length() ), ec );
        if ( ec ) LOG_CRIT << "Error writing error to socket " << ec.message();
      }
    }
  }

  boost::asio::awaitable<void> respond( boost::asio::ip::tcp::socket& socket )
  {
    static constexpr int bufSize = 1024;
    static constexpr auto maxBytes = 8 * 1024 * 1024;
    uint8_t data[bufSize];

    const auto documentSize = [&data]( std::size_t length )
    {
      if ( length < 5 ) return length;

      const auto d = reinterpret_cast<const uint8_t*>( data );
      uint32_t len;
      memcpy( &len, d, sizeof(len) );
      return std::size_t( len );
    };

    std::size_t osize = co_await socket.async_read_some( boost::asio::buffer( data ), use_awaitable );
    const auto docSize = documentSize( osize );

    // echo, noop, ping etc.
    if ( docSize < 5 )
    {
      co_await boost::asio::async_write( socket, boost::asio::buffer( data, docSize ), use_awaitable );
      co_return;
    }

    if ( docSize <= bufSize )
    {
      auto doc = model::Document{ reinterpret_cast<const uint8_t*>( data ), docSize };
      co_await process( socket, doc );
      co_return;
    }

    auto read = osize;
    std::vector<uint8_t> rbuf;
    rbuf.reserve( docSize );
    rbuf.insert( rbuf.end(), data, data + osize );

    while ( docSize < maxBytes && read != docSize )
    {
      osize = co_await socket.async_read_some( boost::asio::buffer( data ), use_awaitable );
      rbuf.insert( rbuf.end(), data, data + osize );
      read += osize;
    }

    auto doc = model::Document{ reinterpret_cast<const uint8_t*>( rbuf.data() ), docSize };
    co_await process( socket, doc );
  }

  boost::asio::awaitable<void> serve( boost::asio::ip::tcp::socket socket )
  {
    try
    {
      for (;;)
      {
        co_await respond( socket );
      }
    }
    catch ( const std::exception& e )
    {
      static const std::string eof{ "End of file" };
      if ( eof != e.what() ) LOG_WARN << "Exception servicing request " << e.what();
    }
  }

  boost::asio::awaitable<void> listener()
  {
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::ip::tcp::acceptor acceptor( executor,
        { boost::asio::ip::tcp::v4(), static_cast<boost::asio::ip::port_type>( model::Configuration::instance().port ) } );
    for (;;)
    {
      boost::asio::ip::tcp::socket socket = co_await acceptor.async_accept( use_awaitable );
      boost::asio::co_spawn( executor, serve( std::move(socket) ), boost::asio::detached );
    }
  }
}

int spt::server::run()
{
  namespace net = boost::asio;

  const auto& configuration = model::Configuration::instance();
  net::io_context ioc{ configuration.threads };

  net::signal_set signals( ioc, SIGINT, SIGTERM );
  signals.async_wait( [&](boost::system::error_code const&, int) { ioc.stop(); } );

  try
  {
    std::vector<std::thread> v;
    v.reserve( configuration.threads  );
    for( auto i = configuration.threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ioc] { ioc.run(); } );
    }

    queue::QueueManager::instance();
    auto poller = queue::Poller{};
    v.emplace_back( std::thread{ &spt::queue::Poller::run, &poller } );

    boost::asio::co_spawn( ioc, coroutine::listener(), boost::asio::detached );

    LOG_INFO << "TCP service started";
    ioc.run();

    LOG_INFO << "TCP service stopping";
    poller.stop();
    for ( auto& t : v ) if ( t.joinable() ) t.join();
    LOG_INFO << "All I/O threads stopped";
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error running service. " << ex.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
