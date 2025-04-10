//
// Created by Rakesh on 25/12/2021.
//

#include "client/client.hpp"
#include "../api/contextholder.hpp"
#include "../common/util/clara.hpp"
#include "../log/NanoLog.hpp"

#include <iostream>
#include <thread>
#include <vector>

#include <boost/asio/signal_set.hpp>

int main( int argc, char const * const * argv )
{
  using clara::Opt;
  std::string server{ "localhost" };
#ifdef __APPLE__
  std::string port{ "2020" };
  std::string logLevel{ "debug" };
#else
  std::string port{ "2000" };
  std::string logLevel{"info"};
#endif
  std::string dir{"/tmp/"};
  bool help = false;

  auto options = clara::Help(help) |
      Opt(server, "localhost")["-s"]["--server"]("Server to connect to (default localhost).") |
      Opt(port, "2000")["-p"]["--port"]("TCP port for the server (default 2000)") |
      Opt(logLevel, "info")["-l"]["--log-level"]("Log level to use [debug|info|warn|critical] (default info).") |
      Opt(dir, "/tmp/")["-o"]["--log-dir"]("Log directory (default /tmp/)");

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

  if ( logLevel == "debug" ) nanolog::set_log_level( nanolog::LogLevel::DEBUG );
  else if ( logLevel == "info" ) nanolog::set_log_level( nanolog::LogLevel::INFO );
  else if ( logLevel == "warn" ) nanolog::set_log_level( nanolog::LogLevel::WARN );
  else if ( logLevel == "critical" ) nanolog::set_log_level( nanolog::LogLevel::CRIT );
  nanolog::initialize( nanolog::GuaranteedLogger(), dir, "mongo-service-shell", false );

  boost::asio::signal_set signals( spt::mongoservice::api::ContextHolder::instance().ioc, SIGINT, SIGTERM );
  signals.async_wait( [&](auto const&, int ) { spt::mongoservice::api::ContextHolder::instance().ioc.stop(); } );

  const auto run = []
  {
    for (;;)
    {
      try
      {
        spt::mongoservice::api::ContextHolder::instance().ioc.run();
        break;
      }
      catch ( std::exception& e )
      {
        LOG_CRIT << "Unhandled exception " << e.what();
        spt::mongoservice::api::ContextHolder::instance().ioc.run();
      }
    }
  };

  std::vector<std::thread> v;
  v.reserve( 1 );
  v.emplace_back( [&run] { run(); } );

  try
  {
    spt::mongoservice::client::run( server, port );
  }
  catch ( const std::exception& ex )
  {
    std::cerr << "Error communicating with server. " << ex.what() << '\n';
  }

  spt::mongoservice::api::ContextHolder::instance().ioc.stop();

  for ( auto&& t : v ) if ( t.joinable() ) t.join();
}
