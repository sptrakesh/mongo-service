//
// Created by Rakesh on 02/06/2020.
//

#pragma once

#include <string>
#include <thread>

namespace spt::model
{
  struct Configuration
  {
    static Configuration& instance()
    {
      static Configuration config;
      return config;
    }

    ~Configuration() = default;
    Configuration( Configuration&& ) = default;
    Configuration& operator=( Configuration&& ) = default;

    Configuration( const Configuration& ) = delete;
    Configuration& operator=( const Configuration& ) = delete;

    std::string metricsDatabase{ "versionHistory" };
    std::string metricsCollection{ "metrics" };
    std::string mongoUri;
    std::string versionHistoryDatabase{ "versionHistory" };
    std::string versionHistoryCollection{ "entities" };
    std::string logLevel{ "info" };
    int port = 2000;
    int threads = std::thread::hardware_concurrency();

    [[nodiscard]] std::string str() const;

  private:
    Configuration() = default;
  };
}
