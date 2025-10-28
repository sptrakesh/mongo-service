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
    // A document expressing the write concern of the delete command.
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    // Specifies the collation to use for the operation.
    VISITABLE(std::optional<Collation>, collation);
    // A document that specifies the index to use to support the query predicate.
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    // Specifies a document with a list of variables.
    VISITABLE(std::optional<bsoncxx::document::value>, let);
    // Specifies a time limit in milliseconds.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTimeMS, {std::nullopt});
    // If `true`, then when a delete statement fails, return without performing the remaining delete statements.
    // If `false`, then when a delete statement fails, continue with the remaining delete statements, if any.
    VISITABLE(std::optional<bool>, ordered);
    END_VISITABLES;
  };
}