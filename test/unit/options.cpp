//
// Created by Rakesh on 10/12/2024.
//

#include <catch2/catch_test_macros.hpp>
#include "../../src/api/options.hpp"

#include <bsoncxx/json.hpp>

#include "../../src/common/util/serialise.hpp"

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>

SCENARIO( "Options API model test suite", "[options]" )
{
  GIVEN( "Various MongoDB operation option models" )
  {
    using namespace spt::mongoservice::api::options;
    using bsoncxx::builder::stream::array;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::open_array;
    using bsoncxx::builder::stream::close_array;
    using bsoncxx::builder::stream::finalize;

    WHEN( "Serialising write concern" )
    {
      auto wc = WriteConcern{};
      wc.tag = "test";
      wc.majority = std::chrono::milliseconds{ 100 };
      wc.timeout = std::chrono::milliseconds{ 250 };
      wc.nodes = 2;
      wc.acknowledgeLevel = WriteConcern::Level::Majority;
      wc.journal = true;

      auto bson = spt::util::marshall( wc );
      auto copy = spt::util::unmarshall<WriteConcern>( bson );

      CHECK_FALSE( visit_struct::traits::ext::is_fully_visitable<WriteConcern>() );
      CHECK( wc.tag == copy.tag );
      CHECK( wc.majority == copy.majority );
      CHECK( wc.timeout == copy.timeout );
      CHECK( wc.nodes == copy.nodes );
      CHECK( wc.acknowledgeLevel == copy.acknowledgeLevel );
      CHECK( wc.journal == copy.journal );
    }

    WHEN( "Serialising read preference" )
    {
      auto rp = ReadPreference{};
      rp.hedge = document{} << "enabled" << true << finalize;
      rp.tags = document{} << "region" << "east" << finalize;
      rp.maxStaleness = std::chrono::seconds{ 120 };
      rp.mode = ReadPreference::ReadMode::Nearest;

      auto bson = spt::util::marshall( rp );
      auto copy = spt::util::unmarshall<ReadPreference>( bson );

      CHECK_FALSE( visit_struct::traits::ext::is_fully_visitable<ReadPreference>() );
      CHECK( rp.hedge == copy.hedge );
      CHECK( rp.tags == copy.tags );
      CHECK( rp.maxStaleness == copy.maxStaleness );
      CHECK( rp.mode == copy.mode );
    }

    AND_WHEN( "Serialising insert" )
    {
      auto opt = Insert{};
      opt.writeConcern = WriteConcern{};
      opt.writeConcern->tag = "test";
      opt.writeConcern->majority = std::chrono::milliseconds{ 100 };
      opt.writeConcern->timeout = std::chrono::milliseconds{ 250 };
      opt.writeConcern->nodes = 2;
      opt.writeConcern->acknowledgeLevel = WriteConcern::Level::Majority;
      opt.writeConcern->journal = true;
      opt.bypassValidation = true;
      opt.ordered = true;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Insert>( bson );

      CHECK( opt.writeConcern == copy.writeConcern );
      CHECK( opt.bypassValidation == copy.bypassValidation );
      CHECK( opt.ordered == copy.ordered );
    }

    AND_WHEN( "Serialising update" )
    {
      auto opt = Update{};
      opt.writeConcern = WriteConcern{};
      opt.writeConcern->tag = "test";
      opt.writeConcern->majority = std::chrono::milliseconds{ 100 };
      opt.writeConcern->timeout = std::chrono::milliseconds{ 250 };
      opt.writeConcern->nodes = 2;
      opt.writeConcern->acknowledgeLevel = WriteConcern::Level::Majority;
      opt.writeConcern->journal = true;
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.arrayFilters = array{} <<
        open_document << "price" << open_document << "$gte" << 10 << close_document << close_document <<
        finalize;
      opt.bypassValidation = true;
      opt.upsert = true;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Update>( bson );

      CHECK( opt.writeConcern == copy.writeConcern );
      CHECK( opt.bypassValidation == copy.bypassValidation );
      CHECK( opt.collation == copy.collation );
      CHECK( opt.upsert == copy.upsert );
      REQUIRE( copy.arrayFilters );
      for ( const auto& filter : copy.arrayFilters->view() )
      {
        REQUIRE( filter.type() == bsoncxx::type::k_document );
        const auto doc = filter.get_document().view();
        REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "price", doc ) );
        const auto price = spt::util::bsonValue<bsoncxx::document::view>( "price", doc );
        REQUIRE( spt::util::bsonValueIfExists<int32_t>( "$gte", price ) );
        CHECK( spt::util::bsonValue<int32_t>( "$gte", price ) == 10 );
      }
    }

    AND_WHEN( "Serialising delete" )
    {
      auto opt = Delete{};
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.writeConcern = WriteConcern{};
      opt.writeConcern->tag = "test";
      opt.writeConcern->majority = std::chrono::milliseconds{ 100 };
      opt.writeConcern->timeout = std::chrono::milliseconds{ 250 };
      opt.writeConcern->nodes = 2;
      opt.writeConcern->acknowledgeLevel = WriteConcern::Level::Majority;
      opt.writeConcern->journal = true;
      opt.hint = document{} << "name" << 1 << finalize;
      opt.let = document{} <<
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

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Delete>( bson );

      auto wc = spt::util::bsonValueIfExists<bsoncxx::document::view>( "writeConcern", bson.view() );
      REQUIRE( wc );
      INFO( bsoncxx::to_json( *wc ) );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "acknowledgeLevel", *wc ) );
      CHECK( spt::util::bsonValue<int32_t>( "acknowledgeLevel", *wc ) == static_cast<int32_t>( std::to_underlying( opt.writeConcern->acknowledgeLevel ) ) );

      CHECK( opt.writeConcern == copy.writeConcern );
      CHECK( opt.collation == copy.collation );
      CHECK( opt.hint == copy.hint );
      CHECK( opt.let == copy.let );
    }

    AND_WHEN( "Serialising find" )
    {
      auto opt = Find{};
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.commentOption = document{} << "locale" << "en" << "strength" << 1 << finalize;
      opt.hint = document{} << "name" << 1 << finalize;
      opt.let = document{} <<
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
      opt.max = document{} << "name" << 1 << finalize;
      opt.min = document{} << "name" << 1 << finalize;
      opt.projection = document{} << "name" << 1 << "_id" << 0 << finalize;
      opt.sort = document{} << "name" << 1 << "_id" << -1 << finalize;
      opt.readPreference = ReadPreference{};
      opt.readPreference->hedge = document{} << "enabled" << true << finalize;
      opt.readPreference->tags = document{} << "region" << "east" << finalize;
      opt.readPreference->maxStaleness = std::chrono::seconds{ 120 };
      opt.readPreference->mode = ReadPreference::ReadMode::Nearest;
      opt.comment = "Unit test";
      opt.maxTime = std::chrono::milliseconds{ 1000 };
      opt.limit = 10000;
      opt.skip = 1000;
      opt.partialResults = false;
      opt.returnKey = true;
      opt.showRecordId = true;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Find>( bson );

      auto rp = spt::util::bsonValueIfExists<bsoncxx::document::view>( "readPreference", bson.view() );
      REQUIRE( rp );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "mode", *rp ) );
      CHECK( spt::util::bsonValue<int32_t>( "mode", *rp ) == static_cast<int32_t>( std::to_underlying( ReadPreference::ReadMode::Nearest ) ) );

      CHECK( opt.collation == copy.collation );
      CHECK( opt.commentOption == copy.commentOption );
      CHECK( opt.hint == copy.hint );
      CHECK( opt.let == copy.let );
      CHECK( opt.max == copy.max );
      CHECK( opt.min == copy.min );
      CHECK( opt.projection == copy.projection );
      CHECK( opt.sort == copy.sort );
      CHECK( opt.readPreference == copy.readPreference );
      CHECK( opt.comment == copy.comment );
      CHECK( opt.maxTime == copy.maxTime );
      CHECK( opt.limit == copy.limit );
      CHECK( opt.skip == copy.skip );
      CHECK( opt.partialResults == copy.partialResults );
      CHECK( opt.returnKey == copy.returnKey );
      CHECK( opt.showRecordId == copy.showRecordId );
    }

    AND_WHEN( "Serialising count" )
    {
      auto opt = Count{};
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.hint = document{} << "name" << 1 << finalize;
      opt.maxTime = std::chrono::milliseconds{ 1000 };
      opt.limit = 10000;
      opt.skip = 1000;
      opt.readPreference = ReadPreference{};
      opt.readPreference->hedge = document{} << "enabled" << true << finalize;
      opt.readPreference->tags = document{} << "region" << "east" << finalize;
      opt.readPreference->maxStaleness = std::chrono::seconds{ 120 };
      opt.readPreference->mode = ReadPreference::ReadMode::Nearest;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Count>( bson );

      CHECK( opt.collation == copy.collation );
      CHECK( opt.hint == copy.hint );
      CHECK( opt.readPreference == copy.readPreference );
      CHECK( opt.maxTime == copy.maxTime );
      CHECK( opt.limit == copy.limit );
      CHECK( opt.skip == copy.skip );
    }

    AND_WHEN( "Serialising distinct" )
    {
      auto opt = Distinct{};
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.maxTime = std::chrono::milliseconds{ 1000 };
      opt.readPreference = ReadPreference{};
      opt.readPreference->hedge = document{} << "enabled" << true << finalize;
      opt.readPreference->tags = document{} << "region" << "east" << finalize;
      opt.readPreference->maxStaleness = std::chrono::seconds{ 120 };
      opt.readPreference->mode = ReadPreference::ReadMode::Nearest;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Distinct>( bson );

      CHECK( opt.collation == copy.collation );
      CHECK( opt.readPreference == copy.readPreference );
      CHECK( opt.maxTime == copy.maxTime );
    }

    AND_WHEN( "Serialising index" )
    {
      auto opt = Index{};
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.weights = document{} << "content" << 10 << "keywords" << 5 << finalize;
      opt.partialFilterExpression = document{} <<
        "status" << "completed" <<
        "total" << open_document << "$gt" << 100 << close_document <<
        finalize;
      opt.name = "statusidx";
      opt.defaultLanguage = "en";
      opt.languageOverride = "en-gb";
      opt.expireAfterSeconds = std::chrono::seconds{ 1000 };
      opt.twodLocationMin = -2.2;
      opt.twodLocationMax = 2.2;
      opt.version = 2;
      opt.twodSphereVersion = 1;
      opt.twodBitsPrecision = 6;
      opt.background = true;
      opt.unique = false;
      opt.hidden = false;
      opt.sparse = true;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<Index>( bson );

      CHECK( opt.collation == copy.collation );
      CHECK( opt.weights == copy.weights );
      CHECK( opt.partialFilterExpression == copy.partialFilterExpression );
      CHECK( opt.name == copy.name );
      CHECK( opt.defaultLanguage == copy.defaultLanguage );
      CHECK( opt.expireAfterSeconds == copy.expireAfterSeconds );
      CHECK( opt.twodLocationMin == copy.twodLocationMin );
      CHECK( opt.twodLocationMax == copy.twodLocationMax );
      CHECK( opt.twodSphereVersion == copy.twodSphereVersion );
      CHECK( opt.twodBitsPrecision == copy.twodBitsPrecision );
      CHECK( opt.background == copy.background );
      CHECK( opt.unique == copy.unique );
      CHECK( opt.hidden == copy.hidden );
      CHECK( opt.sparse == copy.sparse );
    }

    AND_WHEN( "Serialising create collection" )
    {
      auto opt = CreateCollection{};
      opt.writeConcern = WriteConcern{};
      opt.writeConcern->tag = "test";
      opt.writeConcern->majority = std::chrono::milliseconds{ 100 };
      opt.writeConcern->timeout = std::chrono::milliseconds{ 250 };
      opt.writeConcern->nodes = 2;
      opt.writeConcern->acknowledgeLevel = WriteConcern::Level::Majority;
      opt.writeConcern->journal = true;
      opt.collation = Collation{};
      opt.collation->locale = "en";
      opt.collation->strength = 1;
      opt.collation->caseLevel = true;
      opt.collation->caseFirst = "case";
      opt.collation->numericOrdering = true;
      opt.collation->backwards = true;
      opt.timeseries = CreateCollection::Timeseries{};
      opt.timeseries->timeField = "timestamp";
      opt.timeseries->metaField = "tags";
      opt.timeseries->bucketRoundingSeconds = std::chrono::seconds{ 15 };
      opt.timeseries->bucketMaxSpanSeconds = std::chrono::seconds{ 600 };
      opt.timeseries->granularity = CreateCollection::Timeseries::Granularity::hours;
      opt.pipeline = array{} << open_document << "$match" << open_document << close_document << close_document << finalize;
      opt.validator = document{} <<
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
      opt.validationAction = CreateCollection::ValidationAction::error;
      opt.validationLevel = CreateCollection::ValidationLevel::strict;
      opt.indexOptionDefaults = document{} <<
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
      opt.storageEngine = document{} <<
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
      opt.changeStreamPreAndPostImages = CreateCollection::ChangeStream{};
      opt.clusteredIndex = CreateCollection::ClusteredIndex{};
      opt.clusteredIndex->name = "unitTest";
      opt.viewOn = "anotherCollection";
      opt.size = 1'000'000'000;
      opt.max = 100'000'000;
      opt.expireAfterSeconds = std::chrono::seconds{ 365*24*60*60 };
      opt.capped = true;

      auto bson = spt::util::marshall( opt );
      CHECK( spt::util::bsonValueIfExists<bsoncxx::document::view>( "changeStreamPreAndPostImages", bson ) );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "clusteredIndex", bson ) );
      auto ci = spt::util::bsonValue<bsoncxx::document::view>( "clusteredIndex", bson );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "key", ci ) );
      auto key = spt::util::bsonValue<bsoncxx::document::view>( "key", ci );
      REQUIRE( spt::util::bsonValueIfExists<int32_t>( "_id", key ) );
      CHECK( spt::util::bsonValue<int32_t>( "_id", key ) == 1 );
      REQUIRE( spt::util::bsonValueIfExists<bool>( "unique", ci ) );
      CHECK( spt::util::bsonValue<bool>( "unique", ci ) );
      REQUIRE( spt::util::bsonValueIfExists<std::string>( "name", ci ) );
      CHECK( spt::util::bsonValue<std::string>( "name", ci ) == opt.clusteredIndex->name );

      auto copy = spt::util::unmarshall<CreateCollection>( bson );

      CHECK( opt.writeConcern == copy.writeConcern );
      CHECK( opt.collation == copy.collation );
      CHECK( opt.timeseries == copy.timeseries );
      CHECK( opt.validator == copy.validator );
      CHECK( opt.validationAction == copy.validationAction );
      CHECK( opt.validationLevel == copy.validationLevel );
      CHECK( opt.indexOptionDefaults == copy.indexOptionDefaults );
      CHECK( opt.storageEngine == copy.storageEngine );
      CHECK( opt.changeStreamPreAndPostImages == copy.changeStreamPreAndPostImages );
      CHECK( opt.clusteredIndex == copy.clusteredIndex );
      CHECK( opt.viewOn == copy.viewOn );
      CHECK( opt.size == copy.size );
      CHECK( opt.max == copy.max );
      CHECK( opt.expireAfterSeconds == copy.expireAfterSeconds );
      CHECK( opt.capped == copy.capped );

      REQUIRE( copy.pipeline );
      REQUIRE( copy.pipeline->view().begin() != copy.pipeline->view().end() );
      REQUIRE( copy.pipeline->view().begin()->type() == bsoncxx::type::k_document );
      REQUIRE( spt::util::bsonValueIfExists<bsoncxx::document::view>( "$match", copy.pipeline->view().begin()->get_document().view() ) );
    }

    AND_WHEN( "Serialising drop collection" )
    {
      auto opt = DropCollection{};
      opt.writeConcern = WriteConcern{};
      opt.writeConcern->tag = "test";
      opt.writeConcern->majority = std::chrono::milliseconds{ 100 };
      opt.writeConcern->timeout = std::chrono::milliseconds{ 250 };
      opt.writeConcern->nodes = 2;
      opt.writeConcern->acknowledgeLevel = WriteConcern::Level::Majority;
      opt.writeConcern->journal = true;

      auto bson = spt::util::marshall( opt );
      auto copy = spt::util::unmarshall<DropCollection>( bson );

      CHECK( opt.writeConcern == copy.writeConcern );
    }
  }
}