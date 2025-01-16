//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include "../../options/distinct.hpp"
#include "action.hpp"

#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document>
  struct Distinct
  {
    struct Payload
    {
      explicit Payload( Document&& document ) : filter{ std::forward<Document>( document ) } {}
      Payload() = default;
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;
      bool operator==(const Payload&) const = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      BEGIN_VISITABLES(Payload);
      VISITABLE(std::optional<Document>, filter);
      VISITABLE(std::string, field);
      END_VISITABLES;
    };

    explicit Distinct( Document&& document ) : document{ std::forward<Document>( document ) } {}
    Distinct() = default;
    ~Distinct() = default;
    Distinct(Distinct&&) = default;
    Distinct& operator=(Distinct&&) = default;

    Distinct(const Distinct&) = delete;
    Distinct& operator=(const Distinct&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
    }

    BEGIN_VISITABLES(Distinct);
    VISITABLE(std::optional<Payload>, document);
    VISITABLE(std::optional<options::Distinct>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::distinct};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document>
  void populate( Distinct<Document>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document>
  void populate( const Distinct<Document>& model, bsoncxx::builder::stream::document& builder )
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
  void populate( const Distinct<Document>& model, boost::json::object& object )
  {
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", magic_enum::enum_name( model.action ) );
    object.emplace( "skipMetric", model.skipMetric );
  }
}
