//
// MergeForIdd by Rakesh on 13/12/2024.
//

#pragma once

#include "action.hpp"
#include "../../options/update.hpp"

#if defined __has_include
  #if __has_include("../../../common/util/json.hpp")
    #include "../../../common/util/json.hpp"
  #else
    #include <mongo-service/common/util/json.hpp>
  #endif
#endif

#include <bsoncxx/builder/stream/document.hpp>

namespace spt::mongoservice::api::model::request
{
  template <util::Visitable Document, util::Visitable Metadata>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid> && std::constructible_from<Document, bsoncxx::document::view>
  struct MergeForId
  {
    MergeForId() = default;
    ~MergeForId() = default;
    MergeForId(MergeForId&&) = default;
    MergeForId& operator=(MergeForId&&) = default;

    MergeForId(const MergeForId&) = delete;
    MergeForId& operator=(const MergeForId&) = delete;

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

    BEGIN_VISITABLES(MergeForId);
    Document document;
    VISITABLE(std::optional<Metadata>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid> && std::constructible_from<Document, bsoncxx::document::view>
  struct MergeForIdWithReference
  {
    explicit MergeForIdWithReference( const Document& doc ) : document{ std::cref( doc ) } {}
    MergeForIdWithReference( const Document& doc, const Metadata& md ) : document{ std::cref( doc ) }, metadata{ std::cref( md ) } {}
    ~MergeForIdWithReference() = default;
    MergeForIdWithReference(MergeForIdWithReference&&) = default;
    MergeForIdWithReference& operator=(MergeForIdWithReference&&) = default;

    MergeForIdWithReference(const MergeForIdWithReference&) = delete;
    MergeForIdWithReference& operator=(const MergeForIdWithReference&) = delete;

    BEGIN_VISITABLES(MergeForIdWithReference);
    std::reference_wrapper<const Document> document;
    VISITABLE(std::optional<std::reference_wrapper<const Metadata>>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  struct IdFilter
  {
    explicit IdFilter( bsoncxx::document::view bson ) { util::unmarshall( *this, bson ); }
    explicit IdFilter( bsoncxx::oid id ) : id( id ) {}
    IdFilter() = default;
    ~IdFilter() = default;
    IdFilter(IdFilter&&) = default;
    IdFilter& operator=(IdFilter&&) = default;
    bool operator==(const IdFilter&) const = default;

    IdFilter(const IdFilter&) = delete;
    IdFilter& operator=(const IdFilter&) = delete;

    BEGIN_VISITABLES(IdFilter);
    VISITABLE(bsoncxx::oid, id);
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
  struct Replace
  {
    struct Payload
    {
      explicit Payload( bsoncxx::document::view bson ) { util::unmarshall( *this, bson ); }
      Payload() = default;
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      bool operator==(const Payload&) const = default;
      BEGIN_VISITABLES(Payload);
      VISITABLE(std::optional<Filter>, filter);
      VISITABLE(std::optional<Document>, replace);
      END_VISITABLES;
    };

    Replace() = default;
    ~Replace() = default;
    Replace(Replace&&) = default;
    Replace& operator=(Replace&&) = default;

    Replace(const Replace&) = delete;
    Replace& operator=(const Replace&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON_TYPE( bsoncxx::document::view, document, bson, Payload );
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
      FROM_BSON( bool, skipVersion, bson );
    }

    BEGIN_VISITABLES(Replace);
    Payload document;
    VISITABLE(std::optional<Metadata>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
  struct ReplaceWithReference
  {
    struct Payload
    {
      Payload( const Filter& filter, const Document& document ) : filter{ std::cref( filter ) }, replace{ std::cref( document ) } {}
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      bool operator==(const Payload&) const = default;
      BEGIN_VISITABLES(Payload);
      VISITABLE(std::reference_wrapper<const Filter>, filter);
      VISITABLE(std::reference_wrapper<const Document>, replace);
      END_VISITABLES;
    };

    ReplaceWithReference( const Filter& filter, const Document& document ) : document{ filter, document } {}
    ReplaceWithReference( const Filter& filter, const Document& document, const Metadata& metadata ) : document{ filter, document }, metadata{ std::cref( metadata ) } {}
    ~ReplaceWithReference() = default;
    ReplaceWithReference(ReplaceWithReference&&) = default;
    ReplaceWithReference& operator=(ReplaceWithReference&&) = default;

    ReplaceWithReference(const ReplaceWithReference&) = delete;
    ReplaceWithReference& operator=(const ReplaceWithReference&) = delete;

    BEGIN_VISITABLES(ReplaceWithReference);
    Payload document;
    VISITABLE(std::optional<std::reference_wrapper<const Metadata>>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  struct Update
  {
    struct Payload
    {
      explicit Payload( bsoncxx::document::view bson ) { util::unmarshall( *this, bson ); }
      Payload() = default;
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      bool operator==(const Payload&) const = default;
      BEGIN_VISITABLES(Payload);
      VISITABLE(std::optional<Filter>, filter);
      VISITABLE(std::optional<Document>, update);
      END_VISITABLES;
    };

    Update() = default;
    ~Update() = default;
    Update(Update&&) = default;
    Update& operator=(Update&&) = default;

    Update(const Update&) = delete;
    Update& operator=(const Update&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON_TYPE( bsoncxx::document::view, document, bson, Payload );
      FROM_BSON( std::string, database, bson );
      FROM_BSON( std::string, collection, bson );
      FROM_BSON( std::string, application, bson );
      FROM_BSON( std::string, correlationId, bson );
      FROM_BSON( bool, skipMetric, bson );
      FROM_BSON( bool, skipVersion, bson );
    }

    BEGIN_VISITABLES(Update);
    Payload document;
    VISITABLE(std::optional<Metadata>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  struct UpdateWithReference
  {
    struct Payload
    {
      Payload( const Filter& filter, const Document& document ): filter{ std::cref( filter ) }, update{ std::cref( document ) } {}
      ~Payload() = default;
      Payload(Payload&&) = default;
      Payload& operator=(Payload&&) = default;

      Payload(const Payload&) = delete;
      Payload& operator=(const Payload&) = delete;

      bool operator==(const Payload&) const = default;
      BEGIN_VISITABLES(Payload);
      VISITABLE(std::reference_wrapper<const Filter>, filter);
      VISITABLE(std::reference_wrapper<const Document>, update);
      END_VISITABLES;
    };

    UpdateWithReference( const Filter& filter, const Document& document ) : document{ filter, document } {}
    UpdateWithReference( const Filter& filter, const Document& document, const Metadata& metadata ) : document{ filter, document }, metadata{ std::cref( metadata ) } {}
    ~UpdateWithReference() = default;
    UpdateWithReference(UpdateWithReference&&) = default;
    UpdateWithReference& operator=(UpdateWithReference&&) = default;

    UpdateWithReference(const UpdateWithReference&) = delete;
    UpdateWithReference& operator=(const UpdateWithReference&) = delete;

    BEGIN_VISITABLES(UpdateWithReference);
    Payload document;
    VISITABLE(std::optional<std::reference_wrapper<const Metadata>>, metadata);
    VISITABLE(std::optional<options::Update>, options);
    std::string database;
    std::string collection;
    std::string application;
    std::string correlationId;
    Action action{Action::update};
    bool skipVersion{false};
    bool skipMetric{false};
    END_VISITABLES;
  };

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( MergeForId<Document, Metadata>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata>
  void populate( const MergeForId<Document, Metadata>& model, bsoncxx::builder::stream::document& builder )
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
  void populate( const MergeForId<Document, Metadata>& model, boost::json::object& object )
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
  void populate( const MergeForIdWithReference<Document, Metadata>& model, bsoncxx::builder::stream::document& builder )
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
  void populate( const MergeForIdWithReference<Document, Metadata>& model, boost::json::object& object )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( Replace<Document, Metadata, Filter>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const Replace<Document, Metadata, Filter>& model, bsoncxx::builder::stream::document& builder )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const Replace<Document, Metadata, Filter>& model, boost::json::object& object )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const ReplaceWithReference<Document, Metadata, Filter>& model, bsoncxx::builder::stream::document& builder )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const ReplaceWithReference<Document, Metadata, Filter>& model, boost::json::object& object )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( Update<Document, Metadata, Filter>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const Update<Document, Metadata, Filter>& model, bsoncxx::builder::stream::document& builder )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const Update<Document, Metadata, Filter>& model, boost::json::object& object )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const UpdateWithReference<Document, Metadata, Filter>& model, bsoncxx::builder::stream::document& builder )
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

  template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter>
  void populate( const UpdateWithReference<Document, Metadata, Filter>& model, boost::json::object& object )
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
}
