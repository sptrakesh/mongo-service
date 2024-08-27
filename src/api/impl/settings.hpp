//
// Created by Rakesh on 22/02/2022.
//

#pragma once

#include <mutex>
#include <string>
#include <boost/asio/io_context.hpp>

#include "pool/pool.hpp"

namespace spt::mongoservice::api::impl
{
  struct ApiSettings
  {
    static const ApiSettings& instance()
    {
      static ApiSettings s;
      return s;
    }

    std::mutex mutex{};
    std::string server{};
    std::string port{};
    std::string application{};
    pool::Configuration configuration;
    boost::asio::io_context* ioc{ nullptr };

    ~ApiSettings() = default;
    ApiSettings(const ApiSettings&) = delete;
    ApiSettings& operator=(const ApiSettings&) = delete;

  private:
    ApiSettings() = default;
  };
}