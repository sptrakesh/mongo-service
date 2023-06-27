//
// Created by Rakesh on 26/06/2023.
//

#include "metricscollector.h"
#include "pool.h"
#include "../../ilp/builder.h"
#include "../../ilp/ilp.h"

#include <future>
#include <boost/asio/io_context.hpp>

using spt::db::MetricsCollector;
using std::operator""sv;

namespace spt::db::pmetricscollector
{
  struct Client
  {
    static Client& instance()
    {
      static Client c;
      return c;
    }

    void save( std::string&& body )
    {
      client.write( std::move( body ) );
      ioc.run();
    }

  private:
    Client() = default;

    boost::asio::io_context ioc;
    ilp::ILPClient client{ ioc, model::Configuration::instance().ilp->server, model::Configuration::instance().ilp->port };
  };

  void ilp( std::vector<model::Metric> vector )
  {
    if ( vector.empty() ) return;
    try
    {
      const auto& conf = model::Configuration::instance();
      auto builder = ilp::Builder{};

      for ( const auto& metric : vector )
      {
        builder.startRecord( conf.ilp->name ).
            addTag( "action"sv, metric.action ).
            addTag( "database"sv, metric.database ).
            addTag( "collection"sv, metric.collection ).
            addValue( "duration"sv, metric.duration.count() ).
            addValue( "size"sv, static_cast<uint64_t>( metric.size ) ).
            timestamp( std::chrono::duration_cast<std::chrono::nanoseconds>( metric.timestamp.time_since_epoch() ) );

        if ( metric.application ) builder.addTag( "application"sv, *metric.application );
        if ( metric.correlationId ) builder.addTag( "correlationId"sv, *metric.correlationId );
        if ( metric.message ) builder.addTag( "message"sv, *metric.message );
        if ( metric.id ) builder.addTag( "entityId"sv, metric.id->to_string() );
        builder.endRecord();
      }

      Client::instance().save( builder.finish() );
      LOG_INFO << "Saved batch of " << int(vector.size()) << " metrics to " << conf.ilp->server << ".";
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error saving batch of metrics. " << ex.what();
    }
  }

  void mongo( std::vector<model::Metric> vector )
  {
    if ( vector.empty() ) return;
    try
    {
      auto cliento = Pool::instance().acquire();
      if ( !cliento )
      {
        LOG_WARN << "Connection pool exhausted";
        return;
      }

      auto& client = *cliento;
      const auto& conf = model::Configuration::instance();
      auto wl = mongocxx::write_concern{};
      wl.acknowledge_level( mongocxx::write_concern::level::k_unacknowledged );
      auto opts = mongocxx::options::bulk_write{};
      opts.write_concern( wl );
      auto bw = ( *client )[conf.metrics.database][conf.metrics.collection].create_bulk_write( opts );

      for ( const auto& metric : vector )
      {
        bw.append( mongocxx::model::insert_one{ metric.bson() } );
      }
      auto r = bw.execute();
      if ( r ) LOG_INFO << "Saved batch of " << r->inserted_count() << " metrics.";
      else LOG_INFO << "Saved batch of " << int(vector.size()) << " metrics.";
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error saving batch of metrics. " << ex.what();
    }
  }
}

MetricsCollector& MetricsCollector::instance()
{
  static MetricsCollector mc;
  return mc;
}

void MetricsCollector::add( model::Metric&& metric )
{
  auto lock = std::scoped_lock<std::mutex>{ mutex };
  vector.emplace_back( std::move( metric ) );
  const auto& conf = model::Configuration::instance();
  if ( static_cast<int>( vector.size() ) == conf.metrics.batchSize )
  {
    if ( conf.ilp )
    {
      [[maybe_unused]] auto res = std::async( std::launch::async, pmetricscollector::ilp, std::move( vector ) );
    }
    else
    {
      [[maybe_unused]] auto res = std::async( std::launch::async, pmetricscollector::mongo, std::move( vector ) );
    }
    vector = std::vector<model::Metric>{};
    vector.reserve( conf.metrics.batchSize );
  }
}

void MetricsCollector::finish()
{
  LOG_INFO << "Flushing " << int(vector.size()) << " metrics before exit";
  const auto& conf = model::Configuration::instance();
  if ( conf.ilp )
  {
    auto res = std::async( std::launch::async, pmetricscollector::ilp, std::move( vector ) );
    res.wait();
  }
  else
  {
    auto res = std::async( std::launch::async, pmetricscollector::mongo, std::move( vector ) );
    res.wait();
  }
}