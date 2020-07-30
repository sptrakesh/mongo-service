//
// Created by Rakesh on 29/07/2020.
//

#include "metric.h"

#include <bsoncxx/types/value.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <chrono>

bsoncxx::document::value spt::model::Metric::bson() const
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  auto doc = document{};
  doc << "action" << action <<
    "database" << database <<
    "collection" << collection <<
    "time" << duration.count() <<
    "timestamp" << bsoncxx::types::b_int64{
      std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() ).count() };

  if ( id ) doc << "entityId" << *id;
  if ( application ) doc << "application" << *application;
  if ( correlationId ) doc << "correlationId" << *correlationId;
  if ( message ) doc << "message" << *message;

  return doc << finalize;
}
