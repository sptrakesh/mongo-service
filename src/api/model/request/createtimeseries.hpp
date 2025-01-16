//
// CreateTimeseriesd by Rakesh on 13/12/2024.
//

#pragma once

#include "action.hpp"
#include "../../options/insert.hpp"

#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document>
  struct CreateTimeseries
  {
    CreateTimeseries() = default;
    ~CreateTimeseries() = default;
    CreateTimeseries(CreateTimeseries&&) = default;
    CreateTimeseries& operator=(CreateTimeseries&&) = default;

    CreateTimeseries(const CreateTimeseries&) = delete;
    CreateTimeseries& operator=(const CreateTimeseries&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
    }

    BEGIN_VISITABLES(CreateTimeseries);
    VISITABLE(Document, document);
    VISITABLE(std::optional<options::Insert>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::createTimeseries};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document>
  void populate( CreateTimeseries<Document>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document>
  void populate( const CreateTimeseries<Document>& model, bsoncxx::builder::stream::document& builder )
  {
    if ( !model.database.empty() ) builder << "database" << model.database;
    if ( !model.collection.empty() ) builder << "collection" << model.collection;
    if ( !model.application.empty() ) builder << "application" << model.application;
    if ( !model.correlationId.empty() ) builder << "correlationId" << model.correlationId;
    builder <<
      "action" << util::bson( model.action ) <<
      "skipMetric" << model.skipMetric;
  }

  template <util::Visitable Document>
  void populate( const CreateTimeseries<Document>& model, boost::json::object& object )
  {
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", magic_enum::enum_name( model.action ) );
    object.emplace( "skipMetric", model.skipMetric );
  }
}