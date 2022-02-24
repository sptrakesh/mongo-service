//
// Created by Rakesh on 17/05/2020.
//

#include "queuemanager.h"
#include "../log/NanoLog.h"

using spt::queue::QueueManager;
using spt::model::Metric;

QueueManager& QueueManager::instance()
{
  static QueueManager mgr;
  return mgr;
}

QueueManager::QueueManager() : token{ queue }
{
  LOG_INFO << "Queue manager initialised.";
}

void QueueManager::publish( Metric&& metric )
{
  queue.enqueue( std::move( metric ) );
}

bool QueueManager::consume( Metric& metric )
{
  return queue.try_dequeue( token, metric );
}

