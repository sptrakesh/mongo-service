//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "concurrentqueue.h"
#include "model/metric.h"

namespace spt::queue
{
  class QueueManager
  {
  public:
    static QueueManager& instance();

    void publish( model::Metric&& metric );
    bool consume( model::Metric& metric );

    ~QueueManager() = default;
    QueueManager( const QueueManager& ) = delete;
    QueueManager& operator=( const QueueManager& ) = delete;

  private:
    QueueManager();

    moodycamel::ConcurrentQueue<model::Metric> queue;
    moodycamel::ConsumerToken ctoken;
  };
}
