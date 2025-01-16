//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "writeconcern.hpp"

namespace spt::mongoservice::api::options
{
  struct Update
  {
    explicit Update( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Update() = default;
    ~Update() = default;
    Update(Update&&) = default;
    Update& operator=(Update&&) = default;

    Update(const Update&) = delete;
    Update& operator=(const Update&) = delete;

    BEGIN_VISITABLES(Update);
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::array::value>, arrayFilters);
    VISITABLE(std::optional<bool>, bypassValidation);
    VISITABLE(std::optional<bool>, upsert);
    END_VISITABLES;
  };
}
