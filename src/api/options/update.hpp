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
    // Orders the documents before the update is applied.
    VISITABLE(std::optional<bsoncxx::document::value>, sort);
    // An array of filter documents that determine which array elements to modify for
    // an update operation on an array field.
    VISITABLE(std::optional<bsoncxx::array::value>, arrayFilters);
    // Specifies the time limit in milliseconds for the update operation to run before timing out.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTimeMS, {std::nullopt});
    VISITABLE(std::optional<bool>, bypassDocumentValidation);
    // If set to `true`, updates multiple documents that meet the query criteria. If set to `false`,
    // updates one document. The default value is `false`.
    VISITABLE(std::optional<bool>, multi);
    // If `true`, inserts a new document if no document matches the update filter.
    VISITABLE(std::optional<bool>, upsert);
    END_VISITABLES;
  };
}
