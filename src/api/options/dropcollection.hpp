//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "writeconcern.hpp"

namespace spt::mongoservice::api::options
{
  struct DropCollection
  {
    explicit DropCollection( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    DropCollection() = default;
    ~DropCollection() = default;
    DropCollection(DropCollection&&) = default;
    DropCollection& operator=(DropCollection&&) = default;

    DropCollection(const DropCollection&) = delete;
    DropCollection& operator=(const DropCollection&) = delete;

    BEGIN_VISITABLES(DropCollection);
    // A document expressing the write concern.
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    END_VISITABLES;
  };
}
