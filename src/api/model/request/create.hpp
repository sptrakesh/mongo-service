//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include "action.hpp"
#include "../../options/insert.hpp"

#if defined __has_include
  #if __has_include("../../../common/util/json.hpp")
    #include "../../../common/util/json.hpp"
  #else
    #include <mongo-service/common/util/json.hpp>
  #endif
#endif

#include <functional>
#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document, util::Visitable Metadata>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid> && std::constructible_from<Document, bsoncxx::document::view>
  struct Create
  {
    Create() = default;
    ~Create() = default;
    Create(Create&&) = default;
    Create& operator=(Create&&) = default;

    Create(const Create&) = delete;
    Create& operator=(const Create&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON_TYPE( bsoncxx::document::view, document, bson, Document );
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
      FROM_BSON( bool, skipVersion, bson );
    }

    BEGIN_VISITABLES(Create);
    Document document;
    VISITABLE(std::optional<Metadata>, metadata);
    VISITABLE(std::optional<options::Insert>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::create};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid> && std::constructible_from<Document, bsoncxx::document::view>
  struct CreateWithReference
  {
    explicit CreateWithReference( const Document& doc ) : document{ std::cref( doc ) } {}
    CreateWithReference( const Document& doc, const Metadata& md ) : document{ std::cref( doc ) }, metadata{ std::cref( md ) } {}
    ~CreateWithReference() = default;
    CreateWithReference(CreateWithReference&&) = default;
    CreateWithReference& operator=(CreateWithReference&&) = default;

    CreateWithReference(const CreateWithReference&) = delete;
    CreateWithReference& operator=(const CreateWithReference&) = delete;

    BEGIN_VISITABLES(CreateWithReference);
    std::reference_wrapper<const Document> document;
    VISITABLE(std::optional<std::reference_wrapper<const Metadata>>, metadata);
    VISITABLE(std::optional<options::Insert>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::create};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( Create<Document, Metadata>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const Create<Document, Metadata>& model, bsoncxx::builder::stream::document& builder )
  {
    builder << "document" << util::bson( model.document );
    if ( !model.database.empty() ) builder << "database" << model.database;
    if ( !model.collection.empty() ) builder << "collection" << model.collection;
    if ( !model.application.empty() ) builder << "application" << model.application;
    if ( !model.correlationId.empty() ) builder << "correlationId" << model.correlationId;
    builder <<
      "action" << util::bson( model.action ) <<
      "skipVersion" << model.skipVersion <<
      "skipMetric" << model.skipMetric;
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const Create<Document, Metadata>& model, boost::json::object& object )
  {
    object.emplace( "document", util::json::json( model.document ) );
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", util::json::json( model.action ) );
    object.emplace( "skipVersion", model.skipVersion );
    object.emplace( "skipMetric", model.skipMetric );
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const CreateWithReference<Document, Metadata>& model, bsoncxx::builder::stream::document& builder )
  {
    builder << "document" << util::bson( model.document.get() );
    if ( !model.database.empty() ) builder << "database" << model.database;
    if ( !model.collection.empty() ) builder << "collection" << model.collection;
    if ( !model.application.empty() ) builder << "application" << model.application;
    if ( !model.correlationId.empty() ) builder << "correlationId" << model.correlationId;
    builder <<
      "action" << util::bson( model.action ) <<
      "skipVersion" << model.skipVersion <<
      "skipMetric" << model.skipMetric;
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const CreateWithReference<Document, Metadata>& model, boost::json::object& object )
  {
    object.emplace( "document", util::json::json( model.document.get() ) );
    if ( !model.database.empty() ) object.emplace( "database", model.database );
    if ( !model.collection.empty() ) object.emplace( "collection", model.collection );
    if ( !model.application.empty() ) object.emplace( "application", model.application );
    if ( !model.correlationId.empty() ) object.emplace( "correlationId", model.correlationId );
    object.emplace( "action", util::json::json( model.action ) );
    object.emplace( "skipVersion", model.skipVersion );
    object.emplace( "skipMetric", model.skipMetric );
  }
}