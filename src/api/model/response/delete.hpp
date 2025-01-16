//
// Created by Rakesh on 17/12/2024.
//

#pragma once

#include "history.hpp"

#include <vector>

namespace spt::mongoservice::api::model::response
{
  struct Delete
  {
    explicit Delete( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Delete() = default;
    ~Delete() = default;
    Delete(Delete&&) = default;
    Delete& operator=(Delete&&) = default;

    Delete(const Delete&) = delete;
    Delete& operator=(const Delete&) = delete;

    BEGIN_VISITABLES(Delete);
    VISITABLE(std::vector<bsoncxx::oid>, success);
    VISITABLE(std::vector<bsoncxx::oid>, failure);
    VISITABLE(std::vector<History>, history);
    END_VISITABLES;
  };
}
