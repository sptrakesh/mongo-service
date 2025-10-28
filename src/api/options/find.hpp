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
    // Collation settings for operation.
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<bsoncxx::document::value>, commentOption);
    // orces the query optimizer to use specific indexes in the query.
    VISITABLE(std::optional<bsoncxx::document::value>, hint);
    VISITABLE(std::optional<bsoncxx::document::value>, let);
    // The exclusive upper bound for a specific index.
    VISITABLE(std::optional<bsoncxx::document::value>, max);
    // The inclusive lower bound for a specific index.
    VISITABLE(std::optional<bsoncxx::document::value>, min);
    // Specifies the fields to return in the documents that match the query filter.
    VISITABLE(std::optional<bsoncxx::document::value>, projection);
    // The order of the documents returned in the result set. Fields specified in the sort, must have an index.
    VISITABLE(std::optional<bsoncxx::document::value>, sort);
    // Specifies the read preference level for the query.
    VISITABLE(std::optional<ReadPreference>, readPreference);
    // Specifies the read concern level for the query.
    VISITABLE(std::optional<ReadConcern>, readConcern);
    // Adds a $comment to the query that shows in the profiler logs.
    VISITABLE(std::string, comment);
    // The maximum amount of time (in milliseconds) the server should allow the query to run.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, maxTimeMS, {std::nullopt});
    // Sets a limit of documents returned in the result set.
    VISITABLE(std::optional<int64_t>, limit);
    // How many documents to skip before returning the first document in the result set.
    VISITABLE(std::optional<int64_t>, skip);
    // Whether or not pipelines that require more than 100 megabytes of memory to execute write to temporary files on disk.
    VISITABLE(std::optional<bool>, allowDiskUse);
    // For queries against a sharded collection, allows the command (or subsequent getMore commands) to return partial results, rather than an error, if one or more queried shards are unavailable.
    VISITABLE(std::optional<bool>, allowPartialResults);
    // Whether only the index keys are returned for a query.
    VISITABLE(std::optional<bool>, returnKey);
    // If the $recordId field is added to the returned documents. The $recordId indicates the position of the document in the result set.
    VISITABLE(std::optional<bool>, showRecordId);
    END_VISITABLES;
  };
}