//
// Created by Rakesh on 23/02/2022.
//

#pragma once

#include <atomic>

namespace spt::client
{
  struct Status
  {
    static Status& instance()
    {
      static Status s;
      return s;
    }

    ~Status() = default;
    Status(const Status&) = delete;
    Status& operator=(const Status&) = delete;

    std::atomic_int16_t counter{ 0 };

  private:
    Status() = default;
  };
}