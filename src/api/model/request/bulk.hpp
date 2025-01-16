//
// Created by Rakesh on 15/12/2024.
//

#pragma once

#include "action.hpp"

#if defined __has_include
  #if __has_include("../../../common/util/serialise.hpp")
    #include "../../../common/util/serialise.hpp"
    #include "../../../common/util/json.hpp"
    #include "../../../common/visit_struct/visit_struct_intrusive.hpp"
  #else
    #include <mongo-service/common/util/serialise.hpp>
    #include <mongo-service/common/util/json.hpp>
    #include <mongo-service/common/visit_struct/visit_struct_intrusive.hpp>
  #endif
#endif

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Delete>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid> && std::constructible_from<Delete, bsoncxx::document::view>
  struct Bulk
  {
    struct Payload
    {
      Payload() = default;
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      BEGIN_VISITABLES(Payload);
      VISITABLE(std::vector<Document>, insert);
      VISITABLE(std::vector<Delete>, remove);
      END_VISITABLES;
    };

    Bulk() = default;
    ~Bulk() = default;
    Bulk(Bulk&&) = default;
    Bulk& operator=(Bulk&&) = default;

    Bulk(const Bulk&) = delete;
    Bulk& operator=(const Bulk&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
      FROM_BSON( bool, skipVersion, bson );
    }

    BEGIN_VISITABLES(Bulk);
    VISITABLE(Payload, document);
    VISITABLE(std::optional<Metadata>, metadata);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::bulk};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Delete>
  void populate( Bulk<Document, Metadata, Delete>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Delete>
  void populate( const Bulk<Document, Metadata, Delete>& model, bsoncxx::builder::stream::document& builder )
  {
    if ( !model.database.empty() ) builder << "database" << model.database;
    if ( !model.collection.empty() ) builder << "collection" << model.collection;
    if ( !model.application.empty() ) builder << "application" << model.application;
    if ( !model.correlationId.empty() ) builder << "correlationId" << model.correlationId;
    builder <<
      "action" << util::bson( model.action ) <<
      "skipVersion" << model.skipVersion <<
      "skipMetric" << model.skipMetric;
  }

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Delete>
  void populate( const Bulk<Document, Metadata, Delete>& model, boost::json::object& object )
  {
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", util::json::json( model.action ) );
    object.emplace( "skipVersion", model.skipVersion );
    object.emplace( "skipMetric", model.skipMetric );
  }
}
