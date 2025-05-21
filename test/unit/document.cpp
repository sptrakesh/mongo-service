//
// Created by Rakesh on 11/12/2024.
//
#include <catch2/catch_test_macros.hpp>
#include "../../src/api/model.hpp"
#include "../../src/common/util/serialise.hpp"

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

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

    struct Query
    {
      bool operator==(const Query&) const = default;
      BEGIN_VISITABLES(Query);
      VISITABLE(std::string, str);
      END_VISITABLES;
    };

    struct Index
    {
      bool operator==(const Index&) const = default;
      BEGIN_VISITABLES(Index);
      VISITABLE(int32_t, str);
      VISITABLE(std::string, integer);
      VISITABLE(std::string, data);
      END_VISITABLES;
    };
  }
}

SCENARIO( "Document API model test suite", "[document]" )
{
  GIVEN( "Various MongoDB operation models" )
  {
    using namespace spt::mongoservice::api;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    WHEN( "Serialising document" )
    {
      CHECK( visit_struct::traits::ext::is_fully_visitable<pmodel::Document>() );
      auto doc = pmodel::Document{};
      doc.str = "some value";
      const auto copy = spt::util::unmarshall<pmodel::Document>( spt::util::marshall( doc ) );
      CHECK( doc == copy );
    }

    AND_WHEN( "Serialising metadata" )
    {
      CHECK( visit_struct::traits::ext::is_fully_visitable<pmodel::Metadata>() );
      auto doc = pmodel::Metadata{};
      doc.product = "api";
      doc.project = "mongo-service";
      const auto copy = spt::util::unmarshall<pmodel::Metadata>( spt::util::marshall( doc ) );
      CHECK( doc == copy );
    }

    AND_WHEN( "Serialising create model" )
    {
      auto insert = model::request::Create<pmodel::Document, pmodel::Metadata>{};
      insert.database = "test";
      insert.collection = "test";
      insert.application = "unitTest";
      insert.document.str = "value";
      insert.metadata = pmodel::Metadata{};
      insert.metadata->project = "serialisation";
      insert.metadata->product = "mongo-service";
      insert.options = options::Insert{};
      insert.options->writeConcern = options::WriteConcern{};
      insert.options->writeConcern->tag = "test";
      insert.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      insert.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      insert.options->writeConcern->nodes = 2;
      insert.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      insert.options->writeConcern->journal = true;
      insert.options->bypassValidation = true;
      insert.options->ordered = true;

      const auto bson = spt::util::marshall( insert );
      const auto copy = spt::util::unmarshall<model::request::Create<pmodel::Document, pmodel::Metadata>>( bson );

      CHECK( insert.database == copy.database );
      CHECK( insert.collection == copy.collection );
      CHECK( insert.application == copy.application );
      CHECK( insert.document == copy.document );
      CHECK( insert.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( insert.options->writeConcern == copy.options->writeConcern );
      CHECK( insert.options->bypassValidation == copy.options->bypassValidation );
      CHECK( insert.options->ordered == copy.options->ordered );
      CHECK( copy.action == model::request::Action::create );
    }

    AND_WHEN( "Serialising create model with reference" )
    {
      auto doc = pmodel::Document{};
      doc.str = "some value";
      auto md = pmodel::Metadata{};
      md.project = "serialisation";
      md.product = "mongo-service";
      auto insert = model::request::CreateWithReference<pmodel::Document, pmodel::Metadata>{ doc, md };
      insert.database = "test";
      insert.collection = "test";
      insert.application = "unitTest";
      insert.options = options::Insert{};
      insert.options->writeConcern = options::WriteConcern{};
      insert.options->writeConcern->tag = "test";
      insert.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      insert.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      insert.options->writeConcern->nodes = 2;
      insert.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      insert.options->writeConcern->journal = true;
      insert.options->bypassValidation = true;
      insert.options->ordered = true;

      const auto bson = spt::util::marshall( insert );
      const auto copy = spt::util::unmarshall<model::request::Create<pmodel::Document, pmodel::Metadata>>( bson );

      CHECK( insert.database == copy.database );
      CHECK( insert.collection == copy.collection );
      CHECK( insert.application == copy.application );
      CHECK( insert.document == copy.document );
      CHECK( insert.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( insert.options->writeConcern == copy.options->writeConcern );
      CHECK( insert.options->bypassValidation == copy.options->bypassValidation );
      CHECK( insert.options->ordered == copy.options->ordered );
      CHECK( copy.action == model::request::Action::create );
    }

    AND_WHEN( "Serialising retrieve model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto retrieve = model::request::Retrieve<bsoncxx::document::value>{ document{} << "str" << "value" << finalize  };
      retrieve.database = "unit";
      retrieve.collection = "test";
      retrieve.application = "unitTest";
      retrieve.options = options::Find{};
      retrieve.options->collation = options::Collation{};
      retrieve.options->collation->locale = "en";
      retrieve.options->collation->strength = 1;
      retrieve.options->hint = document{} << "name" << 1 << finalize;
      retrieve.options->let = document{} <<
        "vars" <<
          open_document <<
            "total" <<
              open_document <<
                "$add" << open_array << "$price" << "$tax" << close_array <<
              close_document <<
            "discounted" <<
              open_document <<
                "$cond" <<
                  open_document <<
                    "if" << "$applyDiscount" << "then" << 0.9 << "else" << 1 <<
                  close_document <<
              close_document <<
          close_document <<
        "in" <<
          open_document <<
            "$multiply" << open_array << "$$total" << "$$discounted" << close_array <<
          close_document <<
        finalize;
      retrieve.options->projection = document{} << "name" << 1 << "_id" << 0 << finalize;
      retrieve.options->sort = document{} << "name" << 1 << "_id" << -1 << finalize;
      retrieve.options->readPreference = options::ReadPreference{};
      retrieve.options->readPreference->tags = document{} << "region" << "east" << finalize;
      retrieve.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      retrieve.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      retrieve.options->maxTime = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;
      retrieve.options->partialResults = false;
      retrieve.options->returnKey = true;
      retrieve.options->showRecordId = true;

      const auto bson = spt::util::marshall( retrieve );
      const auto copy = spt::util::unmarshall<model::request::Retrieve<bsoncxx::document::value>>( bson );

      CHECK( retrieve.document == copy.document );
      CHECK( retrieve.database == copy.database );
      CHECK( retrieve.collection == copy.collection );
      CHECK( retrieve.application == copy.application );
      CHECK( retrieve.action == copy.action );
      REQUIRE( copy.options );
      CHECK( retrieve.options->collation == copy.options->collation );
      CHECK( retrieve.options->commentOption == copy.options->commentOption );
      CHECK( retrieve.options->hint == copy.options->hint );
      CHECK( retrieve.options->let == copy.options->let );
      CHECK( retrieve.options->max == copy.options->max );
      CHECK( retrieve.options->min == copy.options->min );
      CHECK( retrieve.options->projection == copy.options->projection );
      CHECK( retrieve.options->sort == copy.options->sort );
      CHECK( retrieve.options->readPreference == copy.options->readPreference );
      CHECK( retrieve.options->comment == copy.options->comment );
      CHECK( retrieve.options->maxTime == copy.options->maxTime );
      CHECK( retrieve.options->limit == copy.options->limit );
      CHECK( retrieve.options->skip == copy.options->skip );
      CHECK( retrieve.options->partialResults == copy.options->partialResults );
      CHECK( retrieve.options->returnKey == copy.options->returnKey );
      CHECK( retrieve.options->showRecordId == copy.options->showRecordId );
      CHECK( copy.action == model::request::Action::retrieve );
    }

    AND_WHEN( "Serialising retrieve model with model query" )
    {
      auto retrieve = model::request::Retrieve{ pmodel::Query{ .str = "value" } };
      retrieve.database = "unit";
      retrieve.collection = "test";
      retrieve.application = "unitTest";
      retrieve.options = options::Find{};
      retrieve.options->collation = options::Collation{};
      retrieve.options->collation->locale = "en";
      retrieve.options->collation->strength = 1;
      retrieve.options->hint = document{} << "name" << 1 << finalize;
      retrieve.options->projection = document{} << "name" << 1 << "_id" << 0 << finalize;
      retrieve.options->sort = document{} << "name" << 1 << "_id" << -1 << finalize;
      retrieve.options->readPreference = options::ReadPreference{};
      retrieve.options->readPreference->tags = document{} << "region" << "east" << finalize;
      retrieve.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      retrieve.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      retrieve.options->maxTime = std::chrono::milliseconds{ 1000 };
      retrieve.options->limit = 10000;
      retrieve.options->partialResults = true;
      retrieve.options->returnKey = false;
      retrieve.options->showRecordId = false;

      const auto bson = spt::util::marshall( retrieve );
      const auto copy = spt::util::unmarshall<model::request::Retrieve<pmodel::Query>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == "value" );
      CHECK( retrieve.document == copy.document );
      CHECK( retrieve.database == copy.database );
      CHECK( retrieve.collection == copy.collection );
      CHECK( retrieve.application == copy.application );
      CHECK( retrieve.action == copy.action );
      REQUIRE( copy.options );
      CHECK( retrieve.options->collation == copy.options->collation );
      CHECK( retrieve.options->commentOption == copy.options->commentOption );
      CHECK( retrieve.options->hint == copy.options->hint );
      CHECK( retrieve.options->let == copy.options->let );
      CHECK( retrieve.options->max == copy.options->max );
      CHECK( retrieve.options->min == copy.options->min );
      CHECK( retrieve.options->projection == copy.options->projection );
      CHECK( retrieve.options->sort == copy.options->sort );
      CHECK( retrieve.options->readPreference == copy.options->readPreference );
      CHECK( retrieve.options->comment == copy.options->comment );
      CHECK( retrieve.options->maxTime == copy.options->maxTime );
      CHECK( retrieve.options->limit == copy.options->limit );
      CHECK( retrieve.options->skip == copy.options->skip );
      CHECK( retrieve.options->partialResults == copy.options->partialResults );
      CHECK( retrieve.options->returnKey == copy.options->returnKey );
      CHECK( retrieve.options->showRecordId == copy.options->showRecordId );
      CHECK( copy.action == model::request::Action::retrieve );
    }

    AND_WHEN( "Serialising count model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto count = model::request::Count<bsoncxx::document::value>{ document{} << finalize  };
      count.database = "unit";
      count.collection = "test";
      count.application = "unitTest";
      count.options = options::Count{};
      count.options->collation = options::Collation{};
      count.options->collation->locale = "en";
      count.options->collation->strength = 1;
      count.options->hint = document{} << "name" << 1 << finalize;
      count.options->readPreference = options::ReadPreference{};
      count.options->readPreference->tags = document{} << "region" << "east" << finalize;
      count.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      count.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      count.options->maxTime = std::chrono::milliseconds{ 1000 };
      count.options->limit = 100'000;
      count.options->skip = 200'000;

      const auto bson = spt::util::marshall( count );
      const auto copy = spt::util::unmarshall<model::request::Count<bsoncxx::document::value>>( bson );

      CHECK( count.document == copy.document );
      CHECK( count.database == copy.database );
      CHECK( count.collection == copy.collection );
      CHECK( count.application == copy.application );
      CHECK( count.action == copy.action );
      REQUIRE( copy.options );
      CHECK( count.options->collation == copy.options->collation );
      CHECK( count.options->hint == copy.options->hint );
      CHECK( count.options->readPreference == copy.options->readPreference );
      CHECK( count.options->maxTime == copy.options->maxTime );
      CHECK( count.options->limit == copy.options->limit );
      CHECK( count.options->skip == copy.options->skip );
      CHECK( copy.action == model::request::Action::count );
    }

    AND_WHEN( "Serialising count model with model query" )
    {
      auto count = model::request::Count{ pmodel::Query{ .str = "value" } };
      count.database = "unit";
      count.collection = "test";
      count.application = "unitTest";
      count.options = options::Count{};
      count.options->collation = options::Collation{};
      count.options->collation->locale = "en";
      count.options->collation->strength = 1;
      count.options->hint = document{} << "name" << 1 << finalize;
      count.options->readPreference = options::ReadPreference{};
      count.options->readPreference->tags = document{} << "region" << "east" << finalize;
      count.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      count.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      count.options->maxTime = std::chrono::milliseconds{ 1000 };
      count.options->limit = 10'000;
      count.options->skip = 50'000;

      const auto bson = spt::util::marshall( count );
      const auto copy = spt::util::unmarshall<model::request::Count<pmodel::Query>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == "value" );
      CHECK( count.document == copy.document );
      CHECK( count.database == copy.database );
      CHECK( count.collection == copy.collection );
      CHECK( count.application == copy.application );
      CHECK( count.action == copy.action );
      REQUIRE( copy.options );
      CHECK( count.options->collation == copy.options->collation );
      CHECK( count.options->hint == copy.options->hint );
      CHECK( count.options->readPreference == copy.options->readPreference );
      CHECK( count.options->maxTime == copy.options->maxTime );
      CHECK( count.options->limit == copy.options->limit );
      CHECK( count.options->skip == copy.options->skip );
      CHECK( copy.action == model::request::Action::count );
    }

    AND_WHEN( "Serialising distinct model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto distinct = model::request::Distinct<bsoncxx::document::value>{ document{} << "name" << open_document << "$exists" << true << close_document << finalize  };
      distinct.database = "unit";
      distinct.collection = "test";
      distinct.application = "unitTest";
      distinct.options = options::Distinct{};
      distinct.options->collation = options::Collation{};
      distinct.options->collation->locale = "en";
      distinct.options->collation->strength = 1;
      distinct.options->readPreference = options::ReadPreference{};
      distinct.options->readPreference->tags = document{} << "region" << "east" << finalize;
      distinct.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      distinct.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      distinct.options->maxTime = std::chrono::milliseconds{ 1000 };

      const auto bson = spt::util::marshall( distinct );
      const auto copy = spt::util::unmarshall<model::request::Distinct<bsoncxx::document::value>>( bson );

      REQUIRE( copy.document );
      REQUIRE( copy.document->filter );
      CHECK( spt::util::bsonValueIfExists<bsoncxx::document::view>( "name", *copy.document->filter ) );
      CHECK( distinct.document == copy.document );
      CHECK( distinct.database == copy.database );
      CHECK( distinct.collection == copy.collection );
      CHECK( distinct.application == copy.application );
      CHECK( distinct.action == copy.action );
      REQUIRE( copy.options );
      CHECK( distinct.options->collation == copy.options->collation );
      CHECK( distinct.options->readPreference == copy.options->readPreference );
      CHECK( distinct.options->maxTime == copy.options->maxTime );
      CHECK( copy.action == model::request::Action::distinct );
    }

    AND_WHEN( "Serialising distinct model with model query" )
    {
      auto distinct = model::request::Distinct{ pmodel::Query{ .str = "value" } };
      distinct.database = "unit";
      distinct.collection = "test";
      distinct.application = "unitTest";
      distinct.options = options::Distinct{};
      distinct.options->collation = options::Collation{};
      distinct.options->collation->locale = "en";
      distinct.options->collation->strength = 1;
      distinct.options->readPreference = options::ReadPreference{};
      distinct.options->readPreference->tags = document{} << "region" << "east" << finalize;
      distinct.options->readPreference->maxStaleness = std::chrono::seconds{ 120 };
      distinct.options->readPreference->mode = options::ReadPreference::ReadMode::Nearest;
      distinct.options->maxTime = std::chrono::milliseconds{ 1000 };

      const auto bson = spt::util::marshall( distinct );
      const auto copy = spt::util::unmarshall<model::request::Distinct<pmodel::Query>>( bson );

      const auto doc =  spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      INFO( bsoncxx::to_json( *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "filter", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) == "value" );
      CHECK( distinct.document == copy.document );
      CHECK( distinct.database == copy.database );
      CHECK( distinct.collection == copy.collection );
      CHECK( distinct.application == copy.application );
      CHECK( distinct.action == copy.action );
      REQUIRE( copy.options );
      CHECK( distinct.options->collation == copy.options->collation );
      CHECK( distinct.options->readPreference == copy.options->readPreference );
      CHECK( distinct.options->maxTime == copy.options->maxTime );
      CHECK( copy.action == model::request::Action::distinct );
    }

    AND_WHEN( "Serialising update with merge by id" )
    {
      auto update = model::request::MergeForId<pmodel::Document, pmodel::Metadata>{};
      update.database = "test";
      update.collection = "test";
      update.application = "unitTest";
      update.document.str = "value modified";
      update.metadata = pmodel::Metadata{};
      update.metadata->project = "serialisation";
      update.metadata->product = "mongo-service";
      update.options = options::Update{};
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = true;
      update.options->upsert = true;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::MergeForId<pmodel::Document, pmodel::Metadata>>( bson );

      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.document == copy.document );
      CHECK( update.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->bypassValidation == copy.options->bypassValidation );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising update with merge by id with reference" )
    {
      auto doc = pmodel::Document{};
      doc.str = "value modified";

      auto md = pmodel::Metadata{};
      md.project = "serialisation";
      md.product = "mongo-service";

      auto update = model::request::MergeForIdWithReference<pmodel::Document, pmodel::Metadata>{ doc, md };
      update.database = "test";
      update.collection = "test";
      update.application = "unitTest";
      update.options = options::Update{};
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = true;
      update.options->upsert = true;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::MergeForId<pmodel::Document, pmodel::Metadata>>( bson );

      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.document == copy.document );
      CHECK( update.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->bypassValidation == copy.options->bypassValidation );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising update with replace by id" )
    {
      auto update = model::request::Replace<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{};
      update.database = "test";
      update.collection = "test";
      update.application = "unitTest";
      update.document.filter = model::request::IdFilter{};
      update.document.filter->id = bsoncxx::oid{};
      update.document.replace = pmodel::Document{};
      update.document.replace->str = "value modified";
      update.metadata = pmodel::Metadata{};
      update.metadata->project = "serialisation";
      update.metadata->product = "mongo-service";
      update.options = options::Update{};
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = true;
      update.options->upsert = true;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Replace<pmodel::Document, pmodel::Metadata, model::request::IdFilter>>( bson );

      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.document == copy.document );
      CHECK( update.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->bypassValidation == copy.options->bypassValidation );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising update with replace by id with reference" )
    {
      auto filter = model::request::IdFilter{};
      filter.id = bsoncxx::oid{};

      auto doc = pmodel::Document{};
      doc.str = "value modified";

      auto md = pmodel::Metadata{};
      md.project = "serialisation";
      md.product = "mongo-service";

      auto update = model::request::ReplaceWithReference<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{ filter, doc, md };
      update.database = "test";
      update.collection = "test";
      update.application = "unitTest";
      update.options = options::Update{};
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = true;
      update.options->upsert = true;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Replace<pmodel::Document, pmodel::Metadata, model::request::IdFilter>>( bson );

      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.document.filter == copy.document.filter );
      CHECK( update.document.replace == copy.document.replace );
      CHECK( update.metadata == copy.metadata );
      REQUIRE( copy.options );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->bypassValidation == copy.options->bypassValidation );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising replace model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto update = model::request::Replace<pmodel::Document, pmodel::Metadata, bsoncxx::document::value>{};
      update.document.filter = document{} << "name" << open_document << "$exists" << true << close_document << finalize;
      update.database = "unit";
      update.collection = "test";
      update.application = "unitTest";
      update.document.replace = pmodel::Document{};
      update.document.replace->str = "value modified";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = false;
      update.options->upsert = false;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Replace<pmodel::Document, pmodel::Metadata, bsoncxx::document::value>>( bson );

      CHECK( copy.document.filter );
      CHECK( update.document == copy.document );
      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.action == copy.action );
      REQUIRE( copy.options );
      CHECK( update.options->collation == copy.options->collation );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising replace model with model query" )
    {
      auto update = model::request::Replace<pmodel::Document, pmodel::Metadata, pmodel::Query>{};
      update.document.filter = pmodel::Query{ .str = "value" };
      update.database = "unit";
      update.collection = "test";
      update.application = "unitTest";
      update.document.replace = pmodel::Document{};
      update.document.replace->str = "value modified";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Replace<pmodel::Document, pmodel::Metadata, pmodel::Query>>( bson );

      const auto doc =  spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      INFO( bsoncxx::to_json( *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "filter", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) == "value" );
      CHECK( update.document == copy.document );
      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.action == copy.action );
      REQUIRE( copy.options );
      CHECK( update.options->collation == copy.options->collation );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising update model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto update = model::request::Update<bsoncxx::document::value, pmodel::Metadata, model::request::IdFilter>{};
      update.document.filter = model::request::IdFilter{};
      update.document.filter->id = bsoncxx::oid{};
      update.document.update = document{} <<
        "$unset" << open_document << "obsoleteProperty" << 1 << close_document <<
        "$set" <<
          open_document <<
            "modified" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
            "user._id" << bsoncxx::oid{} <<
          close_document <<
        finalize;
      update.database = "unit";
      update.collection = "test";
      update.application = "unitTest";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = false;
      update.options->upsert = false;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Update<bsoncxx::document::value, pmodel::Metadata, model::request::IdFilter>>( bson );

      const auto doc =  spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      INFO( bsoncxx::to_json( *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "filter", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "update", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "$unset", spt::util::bsonValue<bsoncxx::document::view>( "update", *doc ) ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "$set", spt::util::bsonValue<bsoncxx::document::view>( "update", *doc ) ) );
      CHECK( update.document == copy.document );
      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.action == copy.action );
      REQUIRE( copy.options );
      CHECK( update.options->collation == copy.options->collation );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising update model with bson document and query" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto update = model::request::Update<bsoncxx::document::value, pmodel::Metadata, bsoncxx::document::value>{};
      update.document.filter = document{} << "str" << "value" << finalize;
      update.document.update = document{} <<
        "$unset" << open_document << "obsoleteProperty" << 1 << close_document <<
        "$set" <<
          open_document <<
            "modified" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
            "user._id" << bsoncxx::oid{} <<
          close_document <<
        finalize;
      update.database = "unit";
      update.collection = "test";
      update.application = "unitTest";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = false;
      update.options->upsert = false;

      const auto bson = spt::util::marshall( update );
      const auto copy = spt::util::unmarshall<model::request::Update<bsoncxx::document::value, pmodel::Metadata, bsoncxx::document::value>>( bson );

      const auto doc =  spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      INFO( bsoncxx::to_json( *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "filter", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "filter", *doc ) ) == "value" );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "update", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "$unset", spt::util::bsonValue<bsoncxx::document::view>( "update", *doc ) ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "$set", spt::util::bsonValue<bsoncxx::document::view>( "update", *doc ) ) );
      CHECK( update.document == copy.document );
      CHECK( update.database == copy.database );
      CHECK( update.collection == copy.collection );
      CHECK( update.application == copy.application );
      CHECK( update.action == copy.action );
      REQUIRE( copy.options );
      CHECK( update.options->collation == copy.options->collation );
      CHECK( update.options->writeConcern == copy.options->writeConcern );
      CHECK( update.options->upsert == copy.options->upsert );
      CHECK( copy.action == model::request::Action::update );
    }

    AND_WHEN( "Serialising delete model with bson document" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto remove = model::request::Delete<bsoncxx::document::value, pmodel::Metadata>{ document{} << finalize  };
      remove.database = "unit";
      remove.collection = "test";
      remove.application = "unitTest";
      remove.options = options::Delete{};
      remove.options->collation = options::Collation{};
      remove.options->collation->locale = "en";
      remove.options->collation->strength = 1;
      remove.options->hint = document{} << "name" << 1 << finalize;
      remove.options->writeConcern = options::WriteConcern{};
      remove.options->writeConcern->tag = "test";
      remove.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      remove.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      remove.options->writeConcern->nodes = 2;
      remove.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      remove.options->writeConcern->journal = true;
      remove.options->let = document{} <<
        "vars" <<
          open_document <<
            "total" <<
              open_document <<
                "$add" << open_array << "$price" << "$tax" << close_array <<
              close_document <<
            "discounted" <<
              open_document <<
                "$cond" <<
                  open_document <<
                    "if" << "$applyDiscount" << "then" << 0.9 << "else" << 1 <<
                  close_document <<
              close_document <<
          close_document <<
        "in" <<
          open_document <<
            "$multiply" << open_array << "$$total" << "$$discounted" << close_array <<
          close_document <<
        finalize;

      const auto bson = spt::util::marshall( remove );
      const auto copy = spt::util::unmarshall<model::request::Delete<bsoncxx::document::value, pmodel::Metadata>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<std::string>( "action", bson.view() ) );
      CHECK( spt::util::bsonValue<std::string>( "action", bson.view() ) == "delete" );

      CHECK( remove.document == copy.document );
      CHECK( remove.metadata == copy.metadata );
      CHECK( remove.database == copy.database );
      CHECK( remove.collection == copy.collection );
      CHECK( remove.application == copy.application );
      CHECK( remove.action == copy.action );
      REQUIRE( copy.options );
      CHECK( remove.options->collation == copy.options->collation );
      CHECK( remove.options->hint == copy.options->hint );
      CHECK( remove.options->writeConcern == copy.options->writeConcern );
      CHECK( remove.options->let == copy.options->let );
      CHECK( copy.action == model::request::Action::_delete );
    }

    AND_WHEN( "Serialising remove model with model query" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto remove = model::request::Delete<pmodel::Query, pmodel::Metadata>{ pmodel::Query{ .str = "value" } };
      remove.database = "unit";
      remove.collection = "test";
      remove.application = "unitTest";
      remove.options = options::Delete{};
      remove.options->collation = options::Collation{};
      remove.options->collation->locale = "en";
      remove.options->collation->strength = 1;
      remove.options->hint = document{} << "name" << 1 << finalize;
      remove.options->writeConcern = options::WriteConcern{};
      remove.options->writeConcern->tag = "test";
      remove.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      remove.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      remove.options->writeConcern->nodes = 2;
      remove.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      remove.options->writeConcern->journal = true;
      remove.options->let = document{} <<
        "vars" <<
          open_document <<
            "total" <<
              open_document <<
                "$add" << open_array << "$price" << "$tax" << close_array <<
              close_document <<
            "discounted" <<
              open_document <<
                "$cond" <<
                  open_document <<
                    "if" << "$applyDiscount" << "then" << 0.9 << "else" << 1 <<
                  close_document <<
              close_document <<
          close_document <<
        "in" <<
          open_document <<
            "$multiply" << open_array << "$$total" << "$$discounted" << close_array <<
          close_document <<
        finalize;

      const auto bson = spt::util::marshall( remove );
      const auto copy = spt::util::unmarshall<model::request::Delete<pmodel::Query, pmodel::Metadata>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == "value" );
      CHECK( remove.document == copy.document );
      CHECK( remove.metadata == copy.metadata );
      CHECK( remove.database == copy.database );
      CHECK( remove.collection == copy.collection );
      CHECK( remove.application == copy.application );
      CHECK( remove.action == copy.action );
      REQUIRE( copy.options );
      CHECK( remove.options->collation == copy.options->collation );
      CHECK( remove.options->hint == copy.options->hint );
      CHECK( remove.options->writeConcern == copy.options->writeConcern );
      CHECK( remove.options->let == copy.options->let );
      CHECK( copy.action == model::request::Action::_delete );
    }

    AND_WHEN( "Serialising drop collection model" )
    {
      auto remove = model::request::DropCollection{};
      remove.document.clearVersionHistory = true;
      remove.database = "unit";
      remove.collection = "test";
      remove.application = "unitTest";
      remove.options = options::DropCollection{};
      remove.options->writeConcern = options::WriteConcern{};
      remove.options->writeConcern->tag = "test";
      remove.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      remove.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      remove.options->writeConcern->nodes = 2;
      remove.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      remove.options->writeConcern->journal = true;

      const auto bson = spt::util::marshall( remove );
      const auto copy = spt::util::unmarshall<model::request::DropCollection>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<bool>( "clearVersionHistory", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<bool>( "clearVersionHistory", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( remove.document == copy.document );
      CHECK( remove.database == copy.database );
      CHECK( remove.collection == copy.collection );
      CHECK( remove.application == copy.application );
      CHECK( remove.action == copy.action );
      REQUIRE( copy.options );
      CHECK( remove.options->writeConcern == copy.options->writeConcern );
      CHECK( copy.action == model::request::Action::dropCollection );
    }

    AND_WHEN( "Serialising create index model with bson document" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto index = model::request::Index<bsoncxx::document::value>{ document{} << "str" << -1 << finalize };
      index.database = "unit";
      index.collection = "test";
      index.application = "unitTest";
      index.options = options::Index{};
      index.options->collation = options::Collation{};
      index.options->collation->locale = "en";
      index.options->collation->strength = 1;
      index.options->weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      index.options->partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      index.options->name = "statusidx";
      index.options->defaultLanguage = "en";
      index.options->languageOverride = "en-gb";
      index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
      index.options->twodLocationMin = -2.2;
      index.options->twodLocationMax = 2.2;
      index.options->version = 2;
      index.options->twodSphereVersion = 1;
      index.options->twodBitsPrecision = 6;
      index.options->background = true;
      index.options->unique = false;
      index.options->hidden = false;
      index.options->sparse = true;

      const auto bson = spt::util::marshall( index );
      const auto copy = spt::util::unmarshall<model::request::Index<bsoncxx::document::value>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == -1 );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.options->expireAfterSeconds == copy.options->expireAfterSeconds );
      CHECK( index.options->twodLocationMin == copy.options->twodLocationMin );
      CHECK( index.options->twodLocationMax == copy.options->twodLocationMax );
      CHECK( index.options->twodSphereVersion == copy.options->twodSphereVersion );
      CHECK( index.options->twodBitsPrecision == copy.options->twodBitsPrecision );
      CHECK( index.options->background == copy.options->background );
      CHECK( index.options->unique == copy.options->unique );
      CHECK( index.options->hidden == copy.options->hidden );
      CHECK( index.options->sparse == copy.options->sparse );
      CHECK( copy.action == model::request::Action::index );
    }

    AND_WHEN( "Serialising create index model with custom model" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto index = model::request::Index<pmodel::Index>{};
      index.document = pmodel::Index{ .str = -1, .integer = "2dsphere", .data = "text" };
      index.database = "unit";
      index.collection = "test";
      index.application = "unitTest";
      index.options = options::Index{};
      index.options->collation = options::Collation{};
      index.options->collation->locale = "en";
      index.options->collation->strength = 1;
      index.options->weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      index.options->partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      index.options->name = "statusidx";
      index.options->defaultLanguage = "en";
      index.options->languageOverride = "en-gb";
      index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
      index.options->twodLocationMin = -2.2;
      index.options->twodLocationMax = 2.2;
      index.options->version = 2;
      index.options->twodSphereVersion = 1;
      index.options->twodBitsPrecision = 6;
      index.options->background = true;
      index.options->unique = false;
      index.options->hidden = false;
      index.options->sparse = true;

      const auto bson = spt::util::marshall( index );
      const auto copy = spt::util::unmarshall<model::request::Index<pmodel::Index>>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == -1 );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "integer", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "integer", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == "2dsphere" );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "data", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "data", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == "text" );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.options->expireAfterSeconds == copy.options->expireAfterSeconds );
      CHECK( index.options->twodLocationMin == copy.options->twodLocationMin );
      CHECK( index.options->twodLocationMax == copy.options->twodLocationMax );
      CHECK( index.options->twodSphereVersion == copy.options->twodSphereVersion );
      CHECK( index.options->twodBitsPrecision == copy.options->twodBitsPrecision );
      CHECK( index.options->background == copy.options->background );
      CHECK( index.options->unique == copy.options->unique );
      CHECK( index.options->hidden == copy.options->hidden );
      CHECK( index.options->sparse == copy.options->sparse );
      CHECK( copy.action == model::request::Action::index );
    }

    AND_WHEN( "Serialising drop index model with bson document" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto index = model::request::DropIndex<bsoncxx::document::value>{ document{} << "str" << -1 << finalize };
      index.database = "unit";
      index.collection = "test";
      index.application = "unitTest";
      index.options = options::Index{};
      index.options->collation = options::Collation{};
      index.options->collation->locale = "en";
      index.options->collation->strength = 1;
      index.options->weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      index.options->partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      index.options->name = "statusidx";
      index.options->defaultLanguage = "en";
      index.options->languageOverride = "en-gb";
      index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
      index.options->twodLocationMin = -2.2;
      index.options->twodLocationMax = 2.2;
      index.options->version = 2;
      index.options->twodSphereVersion = 1;
      index.options->twodBitsPrecision = 6;
      index.options->background = true;
      index.options->unique = false;
      index.options->hidden = false;
      index.options->sparse = true;

      const auto bson = spt::util::marshall( index );
      const auto copy = spt::util::unmarshall<model::request::DropIndex<bsoncxx::document::value>>( bson );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "specification", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) );
      CHECK( spt::util::bsonValue<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) == -1 );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.options->expireAfterSeconds == copy.options->expireAfterSeconds );
      CHECK( index.options->twodLocationMin == copy.options->twodLocationMin );
      CHECK( index.options->twodLocationMax == copy.options->twodLocationMax );
      CHECK( index.options->twodSphereVersion == copy.options->twodSphereVersion );
      CHECK( index.options->twodBitsPrecision == copy.options->twodBitsPrecision );
      CHECK( index.options->background == copy.options->background );
      CHECK( index.options->unique == copy.options->unique );
      CHECK( index.options->hidden == copy.options->hidden );
      CHECK( index.options->sparse == copy.options->sparse );
      CHECK( copy.action == model::request::Action::dropIndex );
    }

    AND_WHEN( "Serialising drop index model with custom model" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto index = model::request::DropIndex<pmodel::Index>{ pmodel::Index{ .str = -1, .integer = "2dsphere", .data = "text" } };
      index.database = "unit";
      index.collection = "test";
      index.application = "unitTest";
      index.options = options::Index{};
      index.options->collation = options::Collation{};
      index.options->collation->locale = "en";
      index.options->collation->strength = 1;
      index.options->weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      index.options->partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      index.options->name = "statusidx";
      index.options->defaultLanguage = "en";
      index.options->languageOverride = "en-gb";
      index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
      index.options->twodLocationMin = -2.2;
      index.options->twodLocationMax = 2.2;
      index.options->version = 2;
      index.options->twodSphereVersion = 1;
      index.options->twodBitsPrecision = 6;
      index.options->background = true;
      index.options->unique = false;
      index.options->hidden = false;
      index.options->sparse = true;

      const auto bson = spt::util::marshall( index );
      const auto copy = spt::util::unmarshall<model::request::DropIndex<pmodel::Index>>( bson );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "specification", *doc ) );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) );
      CHECK( spt::util::bsonValue<int32_t>( "str", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) == -1 );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "integer", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) );
      CHECK( spt::util::bsonValue<std::string>( "integer", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) == "2dsphere" );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "data", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) );
      CHECK( spt::util::bsonValue<std::string>( "data", spt::util::bsonValue<bsoncxx::document::view>( "specification", *doc ) ) == "text" );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.options->expireAfterSeconds == copy.options->expireAfterSeconds );
      CHECK( index.options->twodLocationMin == copy.options->twodLocationMin );
      CHECK( index.options->twodLocationMax == copy.options->twodLocationMax );
      CHECK( index.options->twodSphereVersion == copy.options->twodSphereVersion );
      CHECK( index.options->twodBitsPrecision == copy.options->twodBitsPrecision );
      CHECK( index.options->background == copy.options->background );
      CHECK( index.options->unique == copy.options->unique );
      CHECK( index.options->hidden == copy.options->hidden );
      CHECK( index.options->sparse == copy.options->sparse );
      CHECK( copy.action == model::request::Action::dropIndex );
    }

    AND_WHEN( "Serialising drop index model with name" )
    {
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;

      auto index = model::request::DropIndex<bool>{};
      index.document = model::request::DropIndex<bool>::Specification{};
      index.document->name = "statusidx";
      index.database = "unit";
      index.collection = "test";
      index.application = "unitTest";
      index.options = options::Index{};
      index.options->collation = options::Collation{};
      index.options->collation->locale = "en";
      index.options->collation->strength = 1;
      index.options->weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      index.options->partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      index.options->name = "statusidx";
      index.options->defaultLanguage = "en";
      index.options->languageOverride = "en-gb";
      index.options->expireAfterSeconds = std::chrono::seconds{ 1000 };
      index.options->twodLocationMin = -2.2;
      index.options->twodLocationMax = 2.2;
      index.options->version = 2;
      index.options->twodSphereVersion = 1;
      index.options->twodBitsPrecision = 6;
      index.options->background = true;
      index.options->unique = false;
      index.options->hidden = false;
      index.options->sparse = true;

      const auto bson = spt::util::marshall( index );
      const auto copy = spt::util::unmarshall<model::request::DropIndex<bool>>( bson );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "name", *doc ) );
      CHECK( spt::util::bsonValue<std::string>( "name", *doc ) == "statusidx" );
      CHECK( index.document == copy.document );
      CHECK( index.database == copy.database );
      CHECK( index.collection == copy.collection );
      CHECK( index.application == copy.application );
      CHECK( index.action == copy.action );
      CHECK( index.options->collation == copy.options->collation );
      CHECK( index.options->weights == copy.options->weights );
      CHECK( index.options->partialFilterExpression == copy.options->partialFilterExpression );
      CHECK( index.options->name == copy.options->name );
      CHECK( index.options->defaultLanguage == copy.options->defaultLanguage );
      CHECK( index.options->expireAfterSeconds == copy.options->expireAfterSeconds );
      CHECK( index.options->twodLocationMin == copy.options->twodLocationMin );
      CHECK( index.options->twodLocationMax == copy.options->twodLocationMax );
      CHECK( index.options->twodSphereVersion == copy.options->twodSphereVersion );
      CHECK( index.options->twodBitsPrecision == copy.options->twodBitsPrecision );
      CHECK( index.options->background == copy.options->background );
      CHECK( index.options->unique == copy.options->unique );
      CHECK( index.options->hidden == copy.options->hidden );
      CHECK( index.options->sparse == copy.options->sparse );
      CHECK( copy.action == model::request::Action::dropIndex );
    }

    AND_WHEN( "Serialising bulk model with delete model" )
    {
      auto bulk = model::request::Bulk<pmodel::Document, pmodel::Metadata, model::request::IdFilter>{};
      for ( auto i = 0; i < 10; ++i )
      {
        bulk.document.insert.emplace_back();
        bulk.document.remove.emplace_back();
      }
      bulk.database = "unit";
      bulk.collection = "test";
      bulk.application = "unitTest";

      const auto bson = spt::util::marshall( bulk );
      const auto copy = spt::util::unmarshall<model::request::Bulk<pmodel::Document, pmodel::Metadata, model::request::IdFilter>>( bson );

      const auto doc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( doc );

      auto insert = spt::util::bsonValueIfExists<bsoncxx::array::view>( "insert", *doc );
      REQUIRE( insert );
      CHECK( std::distance( insert->begin(), insert->end() ) == int(bulk.document.insert.size()) );

      auto remove = spt::util::bsonValueIfExists<bsoncxx::array::view>( "remove", *doc );
      REQUIRE( remove );
      CHECK( std::distance( remove->begin(), remove->end() ) == int(bulk.document.remove.size()) );

      CHECK( bulk.metadata == copy.metadata );
      CHECK( bulk.database == copy.database );
      CHECK( bulk.collection == copy.collection );
      CHECK( bulk.application == copy.application );
      CHECK( bulk.correlationId == copy.correlationId );
      CHECK( bulk.action == copy.action );
      CHECK( copy.action == model::request::Action::bulk );
    }

    AND_WHEN( "Serialising create timeseries model" )
    {
      auto insert = model::request::CreateTimeseries<pmodel::Document>{};
      insert.database = "test";
      insert.collection = "test";
      insert.application = "unitTest";
      insert.document.str = "value";
      insert.options = options::Insert{};
      insert.options->writeConcern = options::WriteConcern{};
      insert.options->writeConcern->tag = "test";
      insert.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      insert.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      insert.options->writeConcern->nodes = 2;
      insert.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      insert.options->writeConcern->journal = true;
      insert.options->bypassValidation = true;
      insert.options->ordered = true;

      const auto bson = spt::util::marshall( insert );
      const auto copy = spt::util::unmarshall<model::request::CreateTimeseries<pmodel::Document>>( bson );

      CHECK( insert.database == copy.database );
      CHECK( insert.collection == copy.collection );
      CHECK( insert.application == copy.application );
      CHECK( insert.document == copy.document );
      REQUIRE( copy.options );
      CHECK( insert.options->writeConcern == copy.options->writeConcern );
      CHECK( insert.options->bypassValidation == copy.options->bypassValidation );
      CHECK( insert.options->ordered == copy.options->ordered );
      CHECK( copy.action == model::request::Action::createTimeseries );
    }

    AND_WHEN( "Serialising create collection model" )
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
      create.document->changeStreamPreAndPostImages = options::CreateCollection::ChangeStream{};
      create.document->clusteredIndex = options::CreateCollection::ClusteredIndex{};
      create.database = "unit";
      create.collection = "test";
      create.application = "unitTest";
      create.document->writeConcern = options::WriteConcern{};
      create.document->writeConcern->tag = "test";
      create.document->writeConcern->majority = std::chrono::milliseconds{ 100 };
      create.document->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      create.document->writeConcern->nodes = 2;
      create.document->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      create.document->writeConcern->journal = true;
      create.document->collation = options::Collation{};
      create.document->collation->locale = "en";
      create.document->collation->strength = 1;
      create.document->collation->caseLevel = true;
      create.document->collation->caseFirst = "case";
      create.document->collation->numericOrdering = true;
      create.document->collation->backwards = true;
      create.document->pipeline = array{} << open_document << "$match" << open_document << close_document << close_document << finalize;
      create.document->validator = document{} <<
        "$jsonSchema" <<
          open_document <<
            "bsonType" << "object" <<
            "required" << open_array << "name" << "email" << "age" << close_array <<
            "properties" <<
              open_document <<
                "name" <<
                  open_document <<
                    "bsonType" << "string" <<
                    "description" <<  "must be a string and is required" <<
                  close_document <<
                "email" <<
                  open_document <<
                    "bsonType" << "string" <<
                    "pattern" << "^.+@.+\\..+$" <<
                    "description" <<  "must be a valid email address and is required" <<
                  close_document <<
                "age" <<
                  open_document <<
                    "bsonType" << "int" <<
                    "minimum" << 18 <<
                    "maximum" << 65 <<
                    "description" <<  "must be an integer in [18, 65] and is required" <<
                  close_document <<
              close_document <<
          close_document <<
        finalize;
      create.document->validationAction = options::CreateCollection::ValidationAction::error;
      create.document->validationLevel = options::CreateCollection::ValidationLevel::strict;
      create.document->indexOptionDefaults = document{} <<
        "storageEngine" <<
          open_document <<
            "wiredTiger" <<
              open_document <<
                "engineConfig" <<
                  open_document <<
                    "cacheSizeGB" << 2 <<
                    "directoryForIndexes" << true <<
                  close_document <<
                "collectionConfig" << open_document << "blockCompressor" << "zstd" << close_document <<
              close_document <<
          close_document <<
        finalize;
      create.document->storageEngine = document{} <<
        "wiredTiger" <<
          open_document <<
            "engineConfig" <<
              open_document <<
                "cacheSizeGB" << 2 <<
                "directoryForIndexes" << true <<
              close_document <<
            "collectionConfig" << open_document << "blockCompressor" << "zstd" << close_document <<
          close_document <<
        finalize;
      create.document->viewOn = "anotherCollection";
      create.document->size = 1'000'000'000;
      create.document->max = 100'000'000;
      create.document->expireAfterSeconds = std::chrono::seconds{ 365*24*60*60 };
      create.document->capped = true;

      const auto bson = spt::util::marshall( create );
      const auto copy = spt::util::unmarshall<model::request::CreateCollection>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "timeseries", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "changeStreamPreAndPostImages", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "clusteredIndex", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      REQUIRE( copy.document );
      CHECK( create.document->timeseries == copy.document->timeseries );
      CHECK( create.document->changeStreamPreAndPostImages == copy.document->changeStreamPreAndPostImages );
      CHECK( create.document->clusteredIndex == copy.document->clusteredIndex );
      CHECK( create.document->writeConcern == copy.document->writeConcern );
      CHECK( create.document->collation == copy.document->collation );
      CHECK( create.document->validator == copy.document->validator );
      CHECK( create.document->validationAction == copy.document->validationAction );
      CHECK( create.document->validationLevel == copy.document->validationLevel );
      CHECK( create.document->indexOptionDefaults == copy.document->indexOptionDefaults );
      CHECK( create.document->storageEngine == copy.document->storageEngine );
      CHECK( create.document->viewOn == copy.document->viewOn );
      CHECK( create.document->size == copy.document->size );
      CHECK( create.document->max == copy.document->max );
      CHECK( create.document->expireAfterSeconds == copy.document->expireAfterSeconds );
      CHECK( create.document->capped == copy.document->capped );
      CHECK( create.database == copy.database );
      CHECK( create.collection == copy.collection );
      CHECK( create.application == copy.application );
      CHECK( create.action == copy.action );
      CHECK( copy.action == model::request::Action::createCollection );
    }

    AND_WHEN( "Serialising rename collection model" )
    {
      auto rename = model::request::RenameCollection{ "renamed" };
      rename.database = "unit";
      rename.collection = "test";
      rename.application = "unitTest";
      rename.options = options::DropCollection{};
      rename.options->writeConcern = options::WriteConcern{};
      rename.options->writeConcern->tag = "test";
      rename.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      rename.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      rename.options->writeConcern->nodes = 2;
      rename.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      rename.options->writeConcern->journal = true;

      const auto bson = spt::util::marshall( rename );
      const auto copy = spt::util::unmarshall<model::request::RenameCollection>( bson );

      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "target", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) );
      CHECK( spt::util::bsonValue<std::string>( "target", spt::util::bsonValue<bsoncxx::document::view>( "document", bson.view() ) ) == rename.document.target );
      CHECK( rename.document == copy.document );
      CHECK( rename.database == copy.database );
      CHECK( rename.collection == copy.collection );
      CHECK( rename.application == copy.application );
      CHECK( rename.action == copy.action );
      REQUIRE( copy.options );
      CHECK( rename.options->writeConcern == copy.options->writeConcern );
      CHECK( copy.action == model::request::Action::renameCollection );
    }

    AND_WHEN( "Serialising transaction builder" )
    {
      using bsoncxx::builder::stream::open_array;
      using bsoncxx::builder::stream::close_array;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using spt::util::bsonValue;
      using spt::util::bsonValueIfExists;

      auto builder = model::request::TransactionBuilder{ "unit", "test" };

      auto insert = model::request::Create<pmodel::Document, pmodel::Metadata>{};
      insert.database = "test";
      insert.collection = "test";
      insert.application = "unitTest";
      insert.document.str = "value";
      insert.metadata = pmodel::Metadata{};
      insert.metadata->project = "serialisation";
      insert.metadata->product = "mongo-service";
      insert.options = options::Insert{};
      insert.options->writeConcern = options::WriteConcern{};
      insert.options->writeConcern->tag = "test";
      insert.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      insert.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      insert.options->writeConcern->nodes = 2;
      insert.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      insert.options->writeConcern->journal = true;
      insert.options->bypassValidation = true;
      insert.options->ordered = true;
      builder.addCreate( insert );

      auto update = model::request::Update<bsoncxx::document::value, pmodel::Metadata, model::request::IdFilter>{};
      update.document.filter = model::request::IdFilter{};
      update.document.filter->id = bsoncxx::oid{};
      update.document.update = document{} <<
        "$unset" << open_document << "obsoleteProperty" << 1 << close_document <<
        "$set" <<
          open_document <<
            "modified" << bsoncxx::types::b_date{ std::chrono::system_clock::now() } <<
            "user._id" << bsoncxx::oid{} <<
          close_document <<
        finalize;
      update.database = "unit";
      update.collection = "test";
      update.application = "unitTest";
      update.options = options::Update{};
      update.options->collation = options::Collation{};
      update.options->collation->locale = "en";
      update.options->collation->strength = 1;
      update.options->writeConcern = options::WriteConcern{};
      update.options->writeConcern->tag = "test";
      update.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      update.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      update.options->writeConcern->nodes = 2;
      update.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      update.options->writeConcern->journal = true;
      update.options->bypassValidation = false;
      update.options->upsert = false;
      builder.addUpdate( update );

      auto remove = model::request::Delete<bsoncxx::document::value, pmodel::Metadata>{ document{} << finalize  };
      remove.database = "unit";
      remove.collection = "test";
      remove.application = "unitTest";
      remove.options = options::Delete{};
      remove.options->collation = options::Collation{};
      remove.options->collation->locale = "en";
      remove.options->collation->strength = 1;
      remove.options->hint = document{} << "name" << 1 << finalize;
      remove.options->writeConcern = options::WriteConcern{};
      remove.options->writeConcern->tag = "test";
      remove.options->writeConcern->majority = std::chrono::milliseconds{ 100 };
      remove.options->writeConcern->timeout = std::chrono::milliseconds{ 250 };
      remove.options->writeConcern->nodes = 2;
      remove.options->writeConcern->acknowledgeLevel = options::WriteConcern::Level::Majority;
      remove.options->writeConcern->journal = true;
      remove.options->let = document{} <<
        "vars" <<
          open_document <<
            "total" <<
              open_document <<
                "$add" << open_array << "$price" << "$tax" << close_array <<
              close_document <<
            "discounted" <<
              open_document <<
                "$cond" <<
                  open_document <<
                    "if" << "$applyDiscount" << "then" << 0.9 << "else" << 1 <<
                  close_document <<
              close_document <<
          close_document <<
        "in" <<
          open_document <<
            "$multiply" << open_array << "$$total" << "$$discounted" << close_array <<
          close_document <<
        finalize;
      builder.addRemove( remove );

      const auto bson = builder.build();
      REQUIRE( bsonValueIfExists<std::string>( "database", bson.view() ) );
      CHECK( bsonValue<std::string>( "database", bson.view() ) == "unit" );
      REQUIRE( bsonValueIfExists<std::string>( "collection", bson.view() ) );
      CHECK( bsonValue<std::string>( "collection", bson.view() ) == "test" );
      REQUIRE( bsonValueIfExists<std::string>( "application", bson.view() ) );
      CHECK( bsonValue<std::string>( "application", bson.view() ).empty() ); // api not initialised in unit test
      REQUIRE( bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() ) );

      auto doc = bsonValue<bsoncxx::document::view>( "document", bson.view() );
      REQUIRE( bsonValueIfExists<bsoncxx::array::view>( "items", doc ) );

      auto arr = bsonValue<bsoncxx::array::view>( "items", doc );
      REQUIRE( std::distance( arr.begin(), arr.end() ) == 3 );

      auto iter = arr.begin();
      REQUIRE( bsonValueIfExists<std::string>( "action", iter->get_document().value ) );
      CHECK( bsonValue<std::string>( "action", iter->get_document().value ) == magic_enum::enum_name( model::request::Action::create ) );

      std::advance( iter, 1 );
      REQUIRE( bsonValueIfExists<std::string>( "action", iter->get_document().value ) );
      CHECK( bsonValue<std::string>( "action", iter->get_document().value ) == magic_enum::enum_name( model::request::Action::update ) );

      std::advance( iter, 1 );
      REQUIRE( bsonValueIfExists<std::string>( "action", iter->get_document().value ) );
      CHECK( bsonValue<std::string>( "action", iter->get_document().value ) == "delete" );
    }
  }
}
