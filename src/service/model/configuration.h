//
// Created by Rakesh on 02/06/2020.
//

#pragma once

#include <optional>
#include <string>
#include <thread>
#include "../../common/visit_struct/visit_struct_intrusive.hpp"

namespace spt::model
{
  struct Configuration
  {
    static Configuration& instance()
    {
      static Configuration config;
      return config;
    }

    struct Metrics
    {
      BEGIN_VISITABLES(Metrics);
      VISITABLE(std::string, database);
      VISITABLE(std::string, collection);
      VISITABLE(int, batchSize);
      END_VISITABLES;
    };

    struct ILPServer
    {
      BEGIN_VISITABLES(ILPServer);
      VISITABLE(std::string, server);
      VISITABLE(std::string, port);
      VISITABLE(std::string, name);
      END_VISITABLES;
    };

    ~Configuration() = default;
    Configuration( Configuration&& ) = default;
    Configuration& operator=( Configuration&& ) = default;

    Configuration( const Configuration& ) = delete;
    Configuration& operator=( const Configuration& ) = delete;

    BEGIN_VISITABLES(Configuration);
    VISITABLE_DIRECT_INIT(std::optional<ILPServer>, ilp, {std::nullopt});
    VISITABLE_DIRECT_INIT(Metrics, metrics, { .database = "versionHistory", .collection = "metrics", .batchSize = 100});
    VISITABLE(std::string, mongoUri);
    VISITABLE_DIRECT_INIT(std::string, versionHistoryDatabase, {"versionHistory"});
    VISITABLE_DIRECT_INIT(std::string, versionHistoryCollection, {"entities"});
    VISITABLE_DIRECT_INIT(std::string, logLevel, {"info"});
    VISITABLE_DIRECT_INIT(int, port, {2000});
    VISITABLE_DIRECT_INIT(int, threads, {static_cast<int>( std::thread::hardware_concurrency() )});
    END_VISITABLES;

    [[nodiscard]] std::string str() const;

  private:
    Configuration() = default;
  };
}
