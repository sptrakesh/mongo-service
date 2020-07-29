//
// Created by Rakesh on 29/07/2020.
//

#include "metric.h"

#include <bsoncxx/builder/stream/document.hpp>

bsoncxx::document::value spt::model::Metric::bson() const
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::finalize;

  auto doc = document{};
  doc << "action" << action <<
    "database" << database <<
    "collection" << collection <<
    "time" << duration.count();

  if ( id ) doc << "entityId" << *id;
  if ( application ) doc << "application" << *application;
  if ( correlationId ) doc << "correlationId" << *correlationId;
  if ( message ) doc << "message" << *message;

  return doc << finalize;
}
