//
// Created by Rakesh on 17/05/2020.
//

#include "poller.h"
#include "queuemanager.h"
#include "log/NanoLog.h"
#include "model/configuration.h"

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <chrono>

namespace spt::queue::poller
{
  struct MongoClient
  {
    explicit MongoClient( const model::Configuration& configuration ) :
        database{ configuration.metricsDatabase },
        collection{ configuration.metricsCollection },
        client{ std::make_unique<mongocxx::client>( mongocxx::uri{ configuration.mongoUri } ) }
    {
      LOG_INFO << "Connected to mongo";

      mongocxx::write_concern wc;
      wc.acknowledge_level( mongocxx::write_concern::level::k_unacknowledged );
      opts.write_concern( std::move( wc ) );
    }

    void save( const model::Metric& metric )
    {
      const auto bson = metric.bson();
      const auto view = bson.view();

      try
      {
        (*client)[database][collection].insert_one( view, opts );
      }
      catch ( const std::exception& ex )
      {
        LOG_CRIT << "Error saving metric. " << bsoncxx::to_json( view ) << ". " << ex.what();
      }
    }

  private:
    mongocxx::options::insert opts;
    std::string database;
    std::string collection;
    std::unique_ptr<mongocxx::client> client = nullptr;
  };
}

using spt::queue::Poller;

Poller::Poller() = default;
Poller::~Poller() = default;

void Poller::run()
{
  const auto& configuration = model::Configuration::instance();
  mongo = std::make_unique<poller::MongoClient>( configuration );

  running.store( true );
  LOG_INFO << "Metrics queue monitor starting";

  while ( running.load() )
  {
    try
    {
      loop();
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error monitoring metrics queue. " << ex.what();
    }
  }

  LOG_INFO << "Processed " << count << " total metrics from queue";
}

void spt::queue::Poller::stop()
{
  running.store( false );
  LOG_INFO << "Stop requested";
}

void Poller::loop()
{
  auto& queue = QueueManager::instance();
  auto metric = model::Metric{};
  while ( queue.consume( metric ) )
  {
    mongo->save( metric );
    if ( ( ++count % 100 ) == 0 ) LOG_INFO << "Published " << count << " metrics to database.";
  }
  if ( running.load() ) std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

