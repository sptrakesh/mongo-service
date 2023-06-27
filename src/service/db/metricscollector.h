//
// Created by Rakesh on 26/06/2023.
//

#pragma once

#include "../model/configuration.h"
#include "../model/metric.h"

#include <mutex>
#include <vector>

namespace spt::db
{
  struct MetricsCollector
  {
    static MetricsCollector& instance();

    ~MetricsCollector() = default;
    MetricsCollector( MetricsCollector&& ) = delete;
    MetricsCollector& operator=( MetricsCollector&& ) = delete;

    MetricsCollector( const MetricsCollector& ) = delete;
    MetricsCollector& operator=( const MetricsCollector& ) = delete;

    void add( model::Metric&& metric );
    void finish();

  private:
    MetricsCollector() { vector.reserve( model::Configuration::instance().metrics.batchSize ); }

    std::mutex mutex{};
    std::vector<model::Metric> vector{};
  };
}