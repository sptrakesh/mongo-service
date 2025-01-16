//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "readpreference.hpp"

namespace spt::mongoservice::api::options
{
  struct Count
  {
    explicit Count( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Count() = default;
    ~Count() = default;
    Count(Count&&) = default;
    Count& operator=(Count&&) = default;

    Count(const Count&) = delete;
    Count& operator=(const Count&) = delete;

    BEGIN_VISITABLES(Count);
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTime, {std::nullopt});
    VISITABLE(std::optional<int64_t>, limit);
    VISITABLE(std::optional<int64_t>, skip);
    VISITABLE(std::optional<ReadPreference>, readPreference);
    END_VISITABLES;
  };
}