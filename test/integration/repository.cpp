//
// Created by Rakesh on 20/12/2024.
//

#include "../../src/api/repository/repositorywithapm.hpp"

#include <catch2/catch_test_macros.hpp>

namespace
{
  namespace pmodel
  {
    struct Document
    {
      explicit Document( bsoncxx::document::view bson ) { spt::util::unmarshall( *this, bson ); }
      Document() = default;
      ~Document() = default;
      Document(Document&&) = default;
      Document& operator=(Document&&) = default;
      bool operator==(const Document&) const = default;

      Document(const Document&) = delete;
      Document& operator=(const Document&) = delete;

      BEGIN_VISITABLES(Document);
      VISITABLE(bsoncxx::oid, id);
      VISITABLE(std::string, str);
      VISITABLE_DIRECT_INIT(spt::util::DateTimeMs, created, {std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() )});
      VISITABLE_DIRECT_INIT(int64_t, integer, {5});
      VISITABLE_DIRECT_INIT(double, floating, {10.345});
      VISITABLE_DIRECT_INIT(bool, boolean, {true});
      END_VISITABLES;
    };

    struct Metadata
    {
      explicit Metadata( bsoncxx::document::view bson ) { spt::util::unmarshall( *this, bson ); }
      Metadata() = default;
      ~Metadata() = default;
      Metadata(Metadata&&) = default;
      Metadata& operator=(Metadata&&) = default;
      bool operator==(const Metadata&) const = default;

      Metadata(const Metadata&) = delete;
      Metadata& operator=(const Metadata&) = delete;

      BEGIN_VISITABLES(Metadata);
      VISITABLE(std::string, project);
      VISITABLE(std::string, product);
      END_VISITABLES;
    };

    struct TSDocument
    {
      struct Tags
      {
        Tags() = default;
        ~Tags() = default;
        Tags(Tags&&) = default;
        Tags& operator=(Tags&&) = default;
        bool operator==(const Tags&) const = default;

        Tags(const Tags&) = delete;
        Tags& operator=(const Tags&) = delete;

        BEGIN_VISITABLES(Tags);
        VISITABLE(std::string, str);
        VISITABLE_DIRECT_INIT(int64_t, integer, {5});
        VISITABLE_DIRECT_INIT(double, floating, {10.345});
        VISITABLE_DIRECT_INIT(bool, boolean, {true});
        END_VISITABLES;
      };

      TSDocument() = default;
      ~TSDocument() = default;
      TSDocument(TSDocument&&) = default;
      TSDocument& operator=(TSDocument&&) = default;
      bool operator==(const TSDocument&) const = default;

      TSDocument(const TSDocument&) = delete;
      TSDocument& operator=(const TSDocument&) = delete;

      BEGIN_VISITABLES(TSDocument);
      VISITABLE(Tags, tags);
      VISITABLE(bsoncxx::oid, id);
      VISITABLE_DIRECT_INIT(spt::util::DateTimeMs, timestamp, {std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() )});
      END_VISITABLES;
    };

    struct Query
    {
      bool operator==(const Query&) const = default;
      BEGIN_VISITABLES(Query);
      VISITABLE(std::string, str);
      END_VISITABLES;
    };
  }

  namespace prepository
  {
    struct Fixture
    {
      mutable bsoncxx::oid oid;
      std::string database{ "itest" };
      std::string collection{ "test" };
      std::string tscollection{ "testts" };
    };
  }
}

TEST_CASE_PERSISTENT_FIXTURE( prepository::Fixture, "Repository interface test suite", "[repository]" )
{
  GIVEN( "Connected to Mongo Service" )
  {
    using namespace spt::mongoservice::api;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    WHEN( "Creating a document" )
    {
      auto insert = model::request::Create<pmodel::Document, pmodel::Metadata>{};
      insert.database = database;
      insert.collection = collection;
      insert.document.str = "value";
      insert.metadata.emplace();
      insert.metadata->project = "serialisation";
      insert.metadata->product = "mongo-service";
      insert.options.emplace();
      insert.options->bypassValidation = true;
      insert.options->ordered = true;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::create( insert, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      REQUIRE( result.value().entity );
      CHECK_FALSE( insert.application.empty() );
      CHECK( *result.value().entity == insert.document.id );
      CHECK( *result.value().id != insert.document.id );
      oid = *result.value().entity;
    }

    AND_WHEN( "Counting matching documents" )
    {
      auto count = model::request::Count<bsoncxx::document::value>{ document{} << finalize  };
      count.database = database;
      count.collection = collection;
      count.options = options::Count{};
      count.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      count.options->limit = 100'000;

      auto result = repository::count( count );
      REQUIRE( result.has_value() );
      CHECK_FALSE( count.application.empty() );
      CHECK( result.value().count > 0 );
    }

    AND_WHEN( "Updating a document using merge by id" )
    {
      auto update = model::request::MergeForId<pmodel::Document, pmodel::Metadata>{};
      update.database = database;
      update.collection = collection;
      update.document.id = oid;
      update.document.str = "value modified";
      update.metadata = pmodel::Metadata{};
      update.metadata->project = "serialisation";
      update.metadata->product = "mongo-service";
      update.options = options::Update{};
      update.options->bypassDocumentValidation = true;
      update.options->upsert = true;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::update( update, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      REQUIRE( result.value().document );
      CHECK_FALSE( update.application.empty() );
      CHECK( result.value().document->id == oid );
      CHECK( result.value().document->str == update.document.str );
      REQUIRE( result.value().history );
      CHECK( result.value().history->id != oid );
    }

    AND_WHEN( "Replacing a document by id" )
    {
      auto update = model::request::Replace<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{};
      update.database = database;
      update.collection = collection;
      update.document.filter.emplace();
      update.document.filter->id = oid;
      update.document.replace = pmodel::Document{};
      update.document.replace->id = oid;
      update.document.replace->str = "value replaced";
      update.metadata.emplace();
      update.metadata->project = "serialisation";
      update.metadata->product = "mongo-service";
      update.options = options::Update{};
      update.options->bypassDocumentValidation = true;
      update.options->upsert = true;

      auto result = repository::update( update );
      REQUIRE( result.has_value() );
      REQUIRE( result.value().document );
      CHECK_FALSE( update.application.empty() );
      CHECK( result.value().document->id == oid );
      CHECK( result.value().document->str == update.document.replace->str );
      REQUIRE( result.value().history );
      CHECK( result.value().history->id != oid );
    }

    AND_WHEN( "Updating document by id" )
    {
      auto update = model::request::Update<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{};
      update.database = database;
      update.collection = collection;
      update.document.filter = model::request::IdFilter{};
      update.document.filter->id = oid;
      update.document.update = pmodel::Document{};
      update.document.update->str = "value modified";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::update( update, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      REQUIRE( result.value().document );
      CHECK_FALSE( update.application.empty() );
      CHECK( result.value().document->id == oid );
      CHECK( result.value().document->str == update.document.update->str );
      REQUIRE( result.value().history );
      CHECK( result.value().history->id != oid );
    }

    AND_WHEN( "Updating documents by property" )
    {
      auto update = model::request::Update<pmodel::Document, pmodel::Metadata, pmodel::Query>{};
      update.database = database;
      update.collection = collection;
      update.document.filter = pmodel::Query{ .str = "value modified" };
      update.document.update = pmodel::Document{};
      update.document.update->str = "value modified update";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;

      auto result = repository::updateMany( update );
      REQUIRE( result.has_value() );
      CHECK_FALSE( update.application.empty() );
      CHECK_FALSE( result.value().success.empty() );
      CHECK( result.value().failure.empty() );
      CHECK_FALSE( result.value().history.empty() );
    }

    AND_WHEN( "Retrieving document by id" )
    {
      auto retrieve = model::request::Retrieve{ model::request::IdFilter{} };
      retrieve.database = database;
      retrieve.collection = collection;
      retrieve.document->id = oid;
      retrieve.options = options::Find{};
      retrieve.options->sort = document{} << "str" << 1 << "_id" << -1 << finalize;
      retrieve.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;
      retrieve.options->allowPartialResults = true;
      retrieve.options->returnKey = false;
      retrieve.options->showRecordId = false;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::retrieve<pmodel::Document>( retrieve, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      CHECK_FALSE( retrieve.application.empty() );
      REQUIRE( result.value().result );
      CHECK( result.value().result->id == oid );
      CHECK( result.value().result->str == "value modified update" );
      CHECK( result.value().results.empty() );
    }

    AND_WHEN( "Retrieving documents by query model" )
    {
      auto retrieve = model::request::Retrieve{ pmodel::Query{ .str = "value modified update" } };
      retrieve.database = database;
      retrieve.collection = collection;
      retrieve.options = options::Find{};
      retrieve.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;
      retrieve.options->allowPartialResults = true;
      retrieve.options->returnKey = false;
      retrieve.options->showRecordId = false;

      auto result = repository::retrieve<pmodel::Document>( retrieve );
      REQUIRE( result.has_value() );
      CHECK_FALSE( retrieve.application.empty() );
      CHECK_FALSE( result.value().result );
      REQUIRE_FALSE( result.value().results.empty() );
      for ( const auto& res : result.value().results ) CHECK( res.str == retrieve.document->str );
    }

    AND_WHEN( "Retrieving documents by raw query" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto retrieve = model::request::Retrieve<bsoncxx::document::value>{ document{} << "str" << "value modified update" << finalize  };
      retrieve.database = database;
      retrieve.collection = collection;
      retrieve.options = options::Find{};
      retrieve.options->sort = document{} << "str" << 1 << "_id" << -1 << finalize;
      retrieve.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::retrieve<pmodel::Document>( retrieve, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      CHECK_FALSE( retrieve.application.empty() );
      CHECK_FALSE( result.value().result );
      REQUIRE_FALSE( result.value().results.empty() );
      CHECK( result.value().results.front().str == "value modified update" );
      CHECK( result.value().results.back().str == "value modified update" );
    }

    AND_WHEN( "Retrieving documents by query which does not match" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto retrieve = model::request::Retrieve<bsoncxx::document::value>{ document{} << "str" << "value modified update" << finalize  };
      retrieve.database = database;
      retrieve.collection = collection;
      retrieve.options = options::Find{};
      retrieve.options->sort = document{} << "str" << 1 << "_id" << -1 << finalize;
      retrieve.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;
      retrieve.options->skip = 10000;

      auto result = repository::retrieve<pmodel::Document>( retrieve );
      REQUIRE( result.has_value() );
      CHECK_FALSE( retrieve.application.empty() );
      CHECK_FALSE( result.value().result );
      CHECK( result.value().results.empty() );
    }

    AND_WHEN( "Retrieving documents by pipeline" )
    {
      using bsoncxx::builder::stream::array;
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto pipeline = model::request::Pipeline{};
      pipeline.database = database;
      pipeline.collection = collection;
      pipeline.addStage( "$match", document{} << "str" << "value modified update" << finalize );
      pipeline.addStage( "$sort", document{} << "_id" << -1 << finalize );
      pipeline.addStage( "$limit", 1 );
      pipeline.options = options::Find{};
      pipeline.options->maxTimeMS = std::chrono::milliseconds{ 1000 };
      pipeline.options->limit = 10000;
      pipeline.options->allowPartialResults = false;
      pipeline.options->returnKey = true;
      pipeline.options->showRecordId = true;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::pipeline<pmodel::Document>( pipeline, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      CHECK_FALSE( pipeline.application.empty() );
      CHECK_FALSE( result.value().result );
      REQUIRE_FALSE( result.value().results.empty() );
      CHECK( result.value().results.front().str == "value modified update" );
      CHECK( result.value().results.back().str == "value modified update" );
    }

    AND_WHEN( "Retrieving distinct values for a field using dummy model" )
    {
      auto distinct = model::request::Distinct{ pmodel::Query{} };
      distinct.database = database;
      distinct.collection = collection;
      distinct.document->filter = std::nullopt;
      distinct.document->field = "str";
      distinct.options = options::Distinct{};
      distinct.options->maxTimeMS = std::chrono::milliseconds{ 1000 };

      auto result = repository::distinct( distinct );
      REQUIRE( result.has_value() );
      CHECK_FALSE( distinct.application.empty() );
      CHECK_FALSE( result.value().results.empty() );
    }

    AND_WHEN( "Retrieving distinct values for a field using bson query" )
    {
      auto distinct = model::request::Distinct<bsoncxx::document::value>{ document{} << finalize };
      distinct.database = database;
      distinct.collection = collection;
      distinct.document->field = "str";
      distinct.options = options::Distinct{};
      distinct.options->maxTimeMS = std::chrono::milliseconds{ 1000 };

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::distinct( distinct, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      CHECK_FALSE( distinct.application.empty() );
      CHECK_FALSE( result.value().results.empty() );
    }

    AND_WHEN( "Deleting the document" )
    {
      auto remove = model::request::Delete<model::request::IdFilter, pmodel::Metadata>{ model::request::IdFilter{ oid } };
      remove.database = database;
      remove.collection = collection;

      auto result = repository::remove( remove );
      REQUIRE( result.has_value() );
      CHECK_FALSE( remove.application.empty() );
      CHECK( result.value().failure.empty() );
      CHECK_FALSE( result.value().success.empty() );
      CHECK( result.value().success.front() == oid );
    }

    AND_WHEN( "Executing bulk requests" )
    {
      auto bulk = model::request::Bulk<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{};
      for ( auto i = 0; i < 10; ++i )
      {
        bulk.document.insert.emplace_back();
        bulk.document.remove.emplace_back();
        bulk.document.remove.back().id = bulk.document.insert.back().id;
      }
      bulk.database = database;
      bulk.collection = collection;

      auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
      auto result = repository::bulk( bulk, apm );
      CHECK_FALSE( apm.processes.empty() );
      REQUIRE( result.has_value() );
      CHECK_FALSE( bulk.application.empty() );
      CHECK( result.value().create == static_cast<int32_t>( bulk.document.insert.size() ) );
      CHECK( result.value().remove == static_cast<int32_t>( bulk.document.remove.size() ) );
    }

    AND_WHEN( "Working with indexes" )
    {
      WHEN( "Creating an index" )
      {
        using bsoncxx::builder::stream::open_document;
        using bsoncxx::builder::stream::close_document;

        auto index = model::request::Index<bsoncxx::document::value>{ document{} << "str" << -1 << finalize };
        index.database = database;
        index.collection = collection;
        index.options = options::Index{};
        index.options->collation = options::Collation{};
        index.options->collation->locale = "en";
        index.options->collation->strength = 1;
        index.options->name = "statusidx";
        index.options->defaultLanguage = "en";
        index.options->languageOverride = "en-gb";
        index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
        index.options->background = true;
        index.options->unique = false;
        index.options->hidden = false;
        index.options->sparse = true;

        auto result = repository::index( index );
        REQUIRE( result.has_value() );
        CHECK_FALSE( index.application.empty() );
        CHECK( result.value().name == index.options->name );
      }

      AND_WHEN( "Dropping an index" )
      {
        auto index = model::request::DropIndex<bsoncxx::document::value>{ document{} << "str" << -1 << finalize };
        index.database = database;
        index.collection = collection;
        index.options = options::Index{};
        index.options->collation = options::Collation{};
        index.options->collation->locale = "en";
        index.options->collation->strength = 1;
        index.options->name = "statusidx";
        index.options->defaultLanguage = "en";
        index.options->languageOverride = "en-gb";
        index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
        index.options->background = true;
        index.options->unique = false;
        index.options->hidden = false;
        index.options->sparse = false;

        auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
        auto result = repository::dropIndex( index, apm );
        CHECK_FALSE( apm.processes.empty() );
        REQUIRE( result.has_value() );
        CHECK_FALSE( index.application.empty() );
        CHECK( result.value().dropIndex );
      }
    }

    AND_WHEN( "Working with timeseries collection" )
    {
      WHEN( "Creating a timeseries collection" )
      {
        using bsoncxx::builder::stream::array;
        using bsoncxx::builder::stream::open_array;
        using bsoncxx::builder::stream::close_array;
        using bsoncxx::builder::stream::open_document;
        using bsoncxx::builder::stream::close_document;

        auto create = model::request::CreateCollection{ options::CreateCollection{} };
        create.document->timeseries = options::CreateCollection::Timeseries{};
        create.document->timeseries->timeField = "timestamp";
        create.document->timeseries->metaField = "tags";
        create.document->timeseries->granularity = options::CreateCollection::Timeseries::Granularity::hours;
        create.database = database;
        create.collection = tscollection;
        create.document->expireAfterSeconds = std::chrono::seconds{ 365*24*60*60 };

        auto result = repository::collection( create );
        REQUIRE( result.has_value() );
        CHECK_FALSE( create.application.empty() );
        CHECK( result.value().database == database );
        CHECK( result.value().collection == tscollection );
      }

      AND_WHEN( "Creating a timeseries document" )
      {
        auto insert = model::request::CreateTimeseries<pmodel::TSDocument>{};
        insert.database = database;
        insert.collection = tscollection;
        insert.document.tags.str = "value";

        auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
        auto result = repository::create( insert, apm );
        CHECK_FALSE( apm.processes.empty() );
        REQUIRE( result.has_value() );
        CHECK_FALSE( insert.application.empty() );
        REQUIRE( result.value().id );
        CHECK( *result.value().id == insert.document.id );
        CHECK_FALSE( result.value().entity );
      }

      AND_WHEN( "Creating a timeseries document out of sequence" )
      {
        const auto ts = std::chrono::system_clock::now() - std::chrono::days{ 1 };
        auto insert = model::request::CreateTimeseries<pmodel::TSDocument>{};
        insert.document.timestamp = spt::util::DateTimeMs{ std::chrono::duration_cast<std::chrono::milliseconds>( ts.time_since_epoch() ) };
        insert.database = database;
        insert.collection = tscollection;
        insert.document.tags.str = "value";

        auto result = repository::create( insert );
        REQUIRE( result.has_value() );
        CHECK_FALSE( insert.application.empty() );
        REQUIRE( result.value().id );
        CHECK( *result.value().id == insert.document.id );
        CHECK_FALSE( result.value().entity );
      }

      AND_WHEN( "Dropping the timeseries collection" )
      {
        auto remove = model::request::DropCollection{};
        remove.document.clearVersionHistory = true;
        remove.database = database;
        remove.collection = tscollection;

        auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
        auto result = repository::dropCollection( remove, apm );
        CHECK_FALSE( apm.processes.empty() );
        REQUIRE( result.has_value() );
        CHECK_FALSE( remove.application.empty() );
        CHECK( result.value().dropCollection );
      }
    }

    AND_WHEN( "CRD collection" )
    {
      const auto coll = std::string{ "renameMe" };
      const auto ren = std::string{ "dropMe" };

      WHEN( "Creating a collection" )
      {
        auto create = model::request::CreateCollection{ options::CreateCollection{} };
        create.database = database;
        create.collection = coll;

        auto result = repository::collection( create );
        REQUIRE( result.has_value() );
        CHECK_FALSE( create.application.empty() );
        CHECK( result.value().database == database );
        CHECK( result.value().collection == coll );
      }

      AND_WHEN( "Renaming a collection" )
      {
        auto rename = model::request::RenameCollection{ ren };
        rename.database = database;
        rename.collection = coll;

        auto apm = spt::ilp::APMRecord{ bsoncxx::oid{}.to_string() };
        auto result = repository::collection( rename, apm );
        CHECK_FALSE( apm.processes.empty() );
        REQUIRE( result.has_value() );
        CHECK_FALSE( rename.application.empty() );
        CHECK( result.value().database == database );
        CHECK( result.value().collection == ren );
      }

      AND_WHEN( "Dropping the renamed collection" )
      {
        auto remove = model::request::DropCollection{};
        remove.document.clearVersionHistory = true;
        remove.database = database;
        remove.collection = ren;

        auto result = repository::dropCollection( remove );
        REQUIRE( result.has_value() );
        CHECK_FALSE( remove.application.empty() );
        CHECK( result.value().dropCollection );
      }
    }
  }
}
