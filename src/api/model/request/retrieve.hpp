//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include "../../options/find.hpp"
#include "action.hpp"

#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document>
  struct Retrieve
  {
    explicit Retrieve( Document&& document ) : document{ std::forward<Document>( document ) } {}
    Retrieve() = default;
    ~Retrieve() = default;
    Retrieve(Retrieve&&) = default;
    Retrieve& operator=(Retrieve&&) = default;

    Retrieve(const Retrieve&) = delete;
    Retrieve& operator=(const Retrieve&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
    }

    BEGIN_VISITABLES(Retrieve);
    VISITABLE(std::optional<Document>, document);
    VISITABLE(std::optional<options::Find>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::retrieve};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document>
  void populate( Retrieve<Document>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document>
  void populate( const Retrieve<Document>& model, bsoncxx::builder::stream::document& builder )
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
  void populate( const Retrieve<Document>& model, boost::json::object& object )
  {
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", magic_enum::enum_name( model.action ) );
    object.emplace( "skipMetric", model.skipMetric );
  }
}
