//
// Created by Rakesh on 15/07/2021.
//

#include "tasks.hpp"
#include "../../src/api/contextholder.hpp"
#include "../../src/log/NanoLog.hpp"
#include "../../src/common/util/clara.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/json/src.hpp>

#include <iostream>
#include <vector>
#if defined(_WIN32) || defined(WIN32)
#include <filesystem>
#endif

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
#if defined(_WIN32) || defined(WIN32)
  auto str = std::filesystem::temp_directory_path().string();
  str.append( "\\" );
  nanolog::initialize( nanolog::GuaranteedLogger(), str, "mongo-service-client", true );
#else
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-client", true );
#endif

  auto& ctx = spt::mongoservice::api::ContextHolder::instance();
  auto guard = boost::asio::make_work_guard( ctx.ioc.get_executor() );
  boost::asio::signal_set signals( ctx.ioc, SIGINT, SIGTERM );
  signals.async_wait( [&](auto, auto){ ctx.ioc.stop(); } );

  std::vector<std::thread> pool;

  for ( std::size_t i = 0; i < std::thread::hardware_concurrency() - 1; ++i ) pool.emplace_back( [&ctx] { ctx.ioc.run(); } );

  boost::system::error_code ec;
  auto client = spt::client::Client( ctx.ioc, host, port, ec );
  assert( !ec );

  boost::asio::co_spawn( ctx.ioc, spt::client::crud( client ), boost::asio::detached );
  boost::asio::co_spawn( ctx.ioc, spt::client::crud(), boost::asio::detached );
  //boost::asio::co_spawn( ctx.ioc, spt::client::apicrud(), boost::asio::detached );

  ctx.ioc.run();
  for ( auto& t : pool ) if ( t.joinable() ) t.join();
}

