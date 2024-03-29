//
// Created by Rakesh on 29/07/2020.
//

#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <bsoncxx/oid.hpp>
#include <bsoncxx/document/value.hpp>

namespace spt::model
{
  struct Metric
  {
    Metric() = default;
    ~Metric() = default;
    Metric( Metric&& ) = default;
    Metric& operator=( Metric&& ) = default;

    Metric( const Metric& ) = delete;
    Metric& operator=( const Metric& ) = delete;

    bsoncxx::oid _id;
    std::string action;
    std::string database;
    std::string collection;
    std::optional<std::string> application;
    std::optional<std::string> correlationId;
    std::optional<std::string> message;
    std::optional<bsoncxx::oid> id;
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> timestamp = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds duration;
    std::size_t size;

    [[nodiscard]] bsoncxx::document::value bson() const;
  };
}
