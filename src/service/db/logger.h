//
// Created by Rakesh on 2019-05-28.
//

#pragma once

#include <mongocxx/logger.hpp>

namespace spt::db
{
  struct Logger : mongocxx::logger
  {
    void operator()(mongocxx::log_level level,
        bsoncxx::stdx::string_view domain,
        bsoncxx::stdx::string_view message) noexcept override;
  };
}
