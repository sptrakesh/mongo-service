//
// Created by Rakesh on 14/12/2023.
//

#pragma once

#include "../simdjson/simdjson.h"
#include "../../api/pool/pool.h"

namespace spt::util::json::parser
{
  struct Parser
  {
    Parser() = default;
    ~Parser() = default;
    Parser(Parser&&) = default;
    Parser& operator=(Parser&&) = default;

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    simdjson::ondemand::parser& parser() { return *p; }

    [[nodiscard]] bool valid() const { return true; }

  private:
    std::unique_ptr<simdjson::ondemand::parser> p{ std::make_unique<simdjson::ondemand::parser>()};
  };

  inline std::unique_ptr<Parser> create() { return std::make_unique<Parser>(); };

  struct Pool
  {
    static Pool& instance()
    {
      static Pool instance;
      return instance;
    }

    auto acquire() -> auto
    {
      return pool->acquire();
    }

    Pool( const Pool& ) = delete;
    Pool( Pool&& ) = delete;
    Pool& operator=( const Pool& ) = delete;
    Pool& operator=( Pool&& ) = delete;

  private:
    Pool()
    {
      auto config = spt::mongoservice::pool::Configuration{};
      config.maxIdleTime = std::chrono::seconds{ 31536000 };
      config.maxPoolSize = 1024;
      pool = std::make_unique<spt::mongoservice::pool::Pool<Parser>>( create, config );
    }

    ~Pool() = default;

    std::unique_ptr<spt::mongoservice::pool::Pool<Parser>> pool;
  };
}