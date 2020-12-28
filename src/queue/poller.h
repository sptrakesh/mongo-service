//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include <atomic>
#include <memory>

namespace spt::queue
{
  namespace poller
  {
    struct MongoClient;
  }

  class Poller
  {
  public:
    Poller();
    ~Poller();

    Poller( const Poller& ) = delete;
    Poller& operator=( const Poller& ) = delete;

    void run();
    void stop();

  private:
    void loop();

    std::unique_ptr<poller::MongoClient> mongo{ nullptr };
    int64_t count = 0;
    std::atomic_bool running;
  };
}
