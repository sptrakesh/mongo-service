//
// Created by Rakesh on 18/07/2020.
//

#include "service.h"
#include "server.h"
#include "model/configuration.h"
#include "queue/queuemanager.h"
#include "queue/poller.h"
#include "log/NanoLog.h"

#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <vector>


int spt::server::run()
{
  namespace net = boost::asio;

  const auto& configuration = model::Configuration::instance();
  net::io_context ioc{ configuration.threads };

  net::signal_set signals( ioc, SIGINT, SIGTERM );
  signals.async_wait( [&](boost::system::error_code const&, int) { ioc.stop(); } );

  try
  {
    Server s{ ioc };

    std::vector<std::thread> v;
    v.reserve( configuration.threads  );
    for( auto i = configuration.threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ioc] { ioc.run(); } );
    }

    queue::QueueManager::instance();
    auto poller = queue::Poller{};
    v.emplace_back( std::thread{ &spt::queue::Poller::run, &poller } );

    LOG_INFO << "TCP service started";
    ioc.run();

    LOG_INFO << "TCP service stopping";
    poller.stop();
    for ( auto& t : v ) t.join();
    LOG_INFO << "All I/O threads stopped";
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error running service. " << ex.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
