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
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::document::value>, weights);
    VISITABLE(std::optional<bsoncxx::document::value>, partialFilterExpression);
    VISITABLE(std::optional<bsoncxx::document::value>, wildcardProjection);
    VISITABLE(std::string, name);
    VISITABLE(std::string, defaultLanguage);
    VISITABLE(std::string, languageOverride);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, expireAfterSeconds, {std::nullopt});
    VISITABLE(std::optional<double>, twodLocationMin);
    VISITABLE(std::optional<double>, twodLocationMax);
    VISITABLE(std::optional<int32_t>, version);
    VISITABLE(std::optional<int32_t>, textIndexVersion);
    VISITABLE(std::optional<int32_t>, twodSphereVersion);
    VISITABLE(std::optional<int32_t>, twodBitsPrecision);
    VISITABLE(std::optional<bool>, background);
    VISITABLE(std::optional<bool>, unique);
    VISITABLE(std::optional<bool>, hidden);
    VISITABLE(std::optional<bool>, sparse);
    END_VISITABLES;
  };
}