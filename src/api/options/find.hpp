//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "readpreference.hpp"

namespace spt::mongoservice::api::options
{
  struct Find
  {
    explicit Find( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    Find() = default;
    ~Find() = default;
    Find(Find&&) = default;
    Find& operator=(Find&&) = default;

    Find(const Find&) = delete;
    Find& operator=(const Find&) = delete;

    BEGIN_VISITABLES(Find);
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::document::value>, commentOption);
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    VISITABLE(std::optional<bsoncxx::document::value>, let);
    VISITABLE(std::optional<bsoncxx::document::value>, max);
    VISITABLE(std::optional<bsoncxx::document::value>, min);
    VISITABLE(std::optional<bsoncxx::document::value>, projection);
    VISITABLE(std::optional<bsoncxx::document::value>, sort);
    VISITABLE(std::optional<ReadPreference>, readPreference);
    VISITABLE(std::string, comment);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTime, {std::nullopt});
    VISITABLE(std::optional<int64_t>, limit);
    VISITABLE(std::optional<int64_t>, skip);
    VISITABLE(std::optional<bool>, partialResults);
    VISITABLE(std::optional<bool>, returnKey);
    VISITABLE(std::optional<bool>, showRecordId);
    END_VISITABLES;
  };
}