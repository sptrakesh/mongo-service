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
    // Specifies the collation to use for the operation.
    VISITABLE(std::optional<Collation>, collation);
    // The index to use. Specify either the index name as a string or the index specification document.
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    // Specifies a time limit in milliseconds.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTimeMS, {std::nullopt});
    // The maximum number of matching documents to return.
    VISITABLE(std::optional<int64_t>, limit);
    // The number of matching documents to skip before returning results.
    VISITABLE(std::optional<int64_t>, skip);
    // Specifies the read concern.
    VISITABLE(std::optional<ReadConcern>, readConcern);
    END_VISITABLES;
  };
}