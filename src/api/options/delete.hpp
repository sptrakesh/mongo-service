//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "writeconcern.hpp"

namespace spt::mongoservice::api::options
{
  struct Delete
  {
    explicit Delete( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Delete() = default;
    ~Delete() = default;
    Delete(Delete&&) = default;
    Delete& operator=(Delete&&) = default;

    Delete(const Delete&) = delete;
    Delete& operator=(const Delete&) = delete;

    BEGIN_VISITABLES(Delete);
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    VISITABLE(std::optional<bsoncxx::document::value>, let);
    END_VISITABLES;
  };
}