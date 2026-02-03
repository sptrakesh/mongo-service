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
    // The index to use. Specify either the index name as a string or the index specification document.
    VISITABLE(std::optional<bsoncxx::types::bson_value::value>, hint);
    // Orders the documents before the update is applied.
    VISITABLE(std::optional<bsoncxx::document::value>, sort);
    // An array of filter documents that determine which array elements to modify for
    // an update operation on an array field.
    VISITABLE(std::optional<bsoncxx::array::value>, arrayFilters);
    // If `true`, ignores any schema validation rules specified on the collection.  Only applies if
    // `upsert` is set to `true`.
    VISITABLE(std::optional<bool>, bypassDocumentValidation);
    // If `true`, inserts a new document if no document matches the update filter.
    VISITABLE(std::optional<bool>, upsert);
    END_VISITABLES;
  };
}
