//
// Created by Rakesh on 15/07/2021.
//

#include "tasks.h"
#include "../../src/log/NanoLog.h"
#include "../../src/util/clara.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <thread>
#include <vector>

int main( int argc, char const * const * argv )
{
  using clara::Opt;

  std::string host{ "localhost" };
  std::string port{ "2020" };
  bool help = false;

  auto options = clara::Help(help) |
      Opt(host, "localhost")["-s"]["--server"]("Server hostname to connect to") |
      Opt(port, "2020")["-p"]["--port"]("Server port to connect to");

  auto result = options.parse(clara::Args(argc, argv));
  if ( !result )
  {
    std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
    exit( 1 );
  }

  if ( help )
  {
    options.writeToStream( std::cout );
    exit( 0 );
  }

  nanolog::set_log_level( nanolog::LogLevel::DEBUG );
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-client", true );

  const auto n = std::thread::hardware_concurrency();
  boost::asio::io_context ioc{ int( n ) };
  boost::asio::signal_set signals( ioc, SIGINT, SIGTERM );
  signals.async_wait( [&](auto, auto){ ioc.stop(); } );

  std::vector<std::thread> pool;

  for ( std::size_t i = 0; i < n - 1; ++i ) pool.emplace_back( [&ioc] { ioc.run(); } );

  boost::system::error_code ec;
  auto client = spt::client::Client( ioc, host, port, ec );
  assert( !ec );

  boost::asio::co_spawn( ioc, spt::client::crud( client ), boost::asio::detached );

  ioc.run();
  for ( auto& t : pool ) if ( t.joinable() ) t.join();
}

