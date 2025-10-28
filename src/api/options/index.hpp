//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"

namespace spt::mongoservice::api::options
{
  struct Index
  {
    explicit Index( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Index() = default;
    ~Index() = default;
    Index(Index&&) = default;
    Index& operator=(Index&&) = default;

    Index(const Index&) = delete;
    Index& operator=(const Index&) = delete;

    BEGIN_VISITABLES(Index);
    // Specifies the collation for the index.
    VISITABLE(std::optional<Collation>, collation);
    // For text indexes, a document that contains field and weight pairs. The weight is an integer ranging from 1 to 99,999 and denotes the significance of the field relative to the other indexed fields in terms of the score.
    VISITABLE(std::optional<bsoncxx::document::value>, weights);
    // If specified, the index only references documents that match the filter expression.
    VISITABLE(std::optional<bsoncxx::document::value>, partialFilterExpression);
    // Allows users to include or exclude specific field paths from a wildcard index.
    VISITABLE(std::optional<bsoncxx::document::value>, wildcardProjection);
    // Allows users to configure the storage engine on a per-index basis when creating an index.
    VISITABLE(std::optional<bsoncxx::document::value>, storageEngine);
    // The name of the index. If unspecified, MongoDB generates an index name by concatenating the names of the indexed fields and the sort order.
    VISITABLE(std::string, name);
    // For text indexes, the language that determines the list of stop words and the rules for the stemmer and tokenizer.
    VISITABLE(std::string, defaultLanguage);
    // For text indexes, the name of the field, in the collection's documents, that contains the override language for the document.
    VISITABLE(std::string, languageOverride);
    // Specifies a value, in seconds, as a time to live (TTL) to control how long MongoDB retains documents in this collection.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, expireAfterSeconds, {std::nullopt});
    // For 2d indexes, the lower inclusive boundary for the longitude and latitude values. The default value is -180.0.
    VISITABLE(std::optional<double>, twodLocationMin);
    // For 2d indexes, the upper inclusive boundary for the longitude and latitude values. The default value is 180.0.
    VISITABLE(std::optional<double>, twodLocationMax);
    VISITABLE(std::optional<int32_t>, version);
    // The text index version number. Users can use this option to override the default version number.
    VISITABLE(std::optional<int32_t>, textIndexVersion);
    // The 2dsphere index version number.
    VISITABLE(std::optional<int32_t>, twodSphereVersion);
    // For 2d indexes, the number of precision of the stored geohash value of the location data.
    VISITABLE(std::optional<int32_t>, twodBitsPrecision);
    VISITABLE(std::optional<bool>, background);
    // Creates a unique index so that the collection will not accept insertion or update of documents where the index key value matches an existing value in the index.
    VISITABLE(std::optional<bool>, unique);
    // A flag that determines whether the index is hidden from the query planner.
    VISITABLE(std::optional<bool>, hidden);
    // If true, the index only references documents with the specified field. These indexes use less space but behave differently in some situations (particularly sorts).
    VISITABLE(std::optional<bool>, sparse);
    END_VISITABLES;
  };
}