//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "readpreference.hpp"

namespace spt::mongoservice::api::options
{
  struct Distinct
  {
    explicit Distinct( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Distinct() = default;
    ~Distinct() = default;
    Distinct(Distinct&&) = default;
    Distinct& operator=(Distinct&&) = default;

    Distinct(const Distinct&) = delete;
    Distinct& operator=(const Distinct&) = delete;

    BEGIN_VISITABLES(Distinct);
    // Specifies the collation to use for the operation.
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTimeMS, {std::nullopt});
    // Specifies the read concern.
    VISITABLE(std::optional<ReadConcern>, readConcern);
    END_VISITABLES;
  };
}