//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "writeconcern.hpp"

namespace spt::mongoservice::api::options
{
  struct Insert
  {
    explicit Insert( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Insert() = default;
    ~Insert() = default;
    Insert(Insert&&) = default;
    Insert& operator=(Insert&&) = default;

    Insert(const Insert&) = delete;
    Insert& operator=(const Insert&) = delete;

    BEGIN_VISITABLES(Insert);
    // A document expressing the write concern. Omit to use the default write concern.
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    VISITABLE(std::optional<bool>, bypassValidation);
    // If true, perform an ordered insert of the documents in the array, and if an error occurs with one of documents, MongoDB will return without processing the remaining documents in the array.
    VISITABLE(std::optional<bool>, ordered);
    END_VISITABLES;
  };
}