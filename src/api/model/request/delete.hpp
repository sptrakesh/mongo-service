//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include "../../options/delete.hpp"
#include "action.hpp"

#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document, util::Visitable Metadata>
  requires std::constructible_from<Metadata, bsoncxx::document::view>
  struct Delete
  {
    explicit Delete( Document&& document ) : document{ std::forward<Document>( document ) } {}
    Delete() = default;
    ~Delete() = default;
    Delete(Delete&&) = default;
    Delete& operator=(Delete&&) = default;

    Delete(const Delete&) = delete;
    Delete& operator=(const Delete&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON_TYPE( bsoncxx::document::view, metadata, bson, Metadata );
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
    }

    BEGIN_VISITABLES(Delete);
    VISITABLE(std::optional<Document>, document);
    VISITABLE(std::optional<options::Delete>, options);
    std::optional<Metadata> metadata;
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::_delete};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( Delete<Document, Metadata>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const Delete<Document, Metadata>& model, bsoncxx::builder::stream::document& builder )
  {
    if ( model.metadata ) builder << "metadata" << util::bson( *model.metadata );
    if ( !model.database.empty() ) builder << "database" << model.database;
    if ( !model.collection.empty() ) builder << "collection" << model.collection;
    if ( !model.application.empty() ) builder << "application" << model.application;
    if ( !model.correlationId.empty() ) builder << "correlationId" << model.correlationId;
    builder <<
      "action" << util::bson( model.action ) <<
      "skipMetric" << model.skipMetric;
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const Delete<Document, Metadata>& model, boost::json::object& object )
  {
    if ( model.metadata ) object.emplace( "metadata", util::json::json( *model.metadata ) );
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", util::json::json( model.action ) );
    object.emplace( "skipMetric", model.skipMetric );
  }
}
