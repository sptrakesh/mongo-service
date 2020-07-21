//
// Created by Rakesh on 19/07/2020.
//

#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

namespace spt::db
{
  struct Pool
  {
    static Pool& instance();
    mongocxx::pool::entry acquire();

    Pool( const Pool& ) = delete;
    Pool& operator=( const Pool& ) = delete;

  private:
    Pool();
    ~Pool() = default;

    void index();

    std::unique_ptr<mongocxx::pool> pool;
  };
}
