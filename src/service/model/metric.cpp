//
// Created by Rakesh on 29/07/2020.
//

#include "metric.hpp"

#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <chrono>

bsoncxx::document::value spt::model::Metric::bson() const
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>( timestamp.time_since_epoch() );
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( timestamp.time_since_epoch() );

  auto doc = document{};
  doc << "_id" << _id <<
    "action" << action <<
    "database" << database <<
    "collection" << collection <<
    "size" << int64_t( size ) <<
    "time" << duration.count() <<
    "timestamp" << bsoncxx::types::b_int64{ ns.count() } <<
    "date" << bsoncxx::types::b_date{ ms };

  if ( id ) doc << "entityId" << *id;
  if ( application ) doc << "application" << *application;
  if ( correlationId ) doc << "correlationId" << *correlationId;
  if ( message ) doc << "message" << *message;

  return doc << finalize;
}
