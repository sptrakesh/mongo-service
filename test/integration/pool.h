//
// Created by Rakesh on 11/08/2020.
//

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace spt::pool
{
  template <typename Connection>
  struct Pool {
    using Ptr = std::unique_ptr<Connection>;
    using Factory = std::function<Ptr()>;

    struct Proxy
    {
      explicit Proxy( Ptr c, Pool<Connection>* p ) :
        con{ std::move( c ) }, pool{ p } {}

      ~Proxy()
      {
        if ( con ) pool->release( std::move( con ) );
      }

      Proxy( Proxy&& p )
      {
        con = std::move( p.con );
        p.con = nullptr;
        pool = p.pool;
        p.pool = nullptr;
      }

      Proxy& operator=( Proxy&& p )
      {
        con = std::move( p.con );
        p.con = nullptr;
        pool = p.pool;
        p.pool = nullptr;
      }

      Proxy(const Proxy&) = delete;
      Proxy& operator=(const Proxy&) = delete;

      Connection& operator*() { return *con.get(); }
      Connection* operator->() { return con.get(); }

    private:
      Ptr con;
      Pool<Connection>* pool;
    };

    explicit Pool( Factory c, uint32_t initial = 1, uint32_t max = 25 ) :
      creator{ c }, initialSize{ initial }, maxSize{ max }
    {
      for ( uint32_t i = 0; i < initialSize; ++i )
      {
        available.emplace_back( c() );
      }
    }

    Pool( const Pool& ) = delete;
    Pool& operator=( const Pool& ) = delete;

    std::optional<Proxy> acquire()
    {
      if ( total >= maxSize ) return std::nullopt;

      auto lock = std::lock_guard( mutex );
      ++total;

      if ( available.empty() ) return Proxy{ creator(), this };

      auto con = std::move( available.front() );
      available.pop_front();
      return Proxy{ std::move( con ), this };
    }

    void release( Ptr c )
    {
      auto lock = std::lock_guard( mutex );
      --total;
      available.emplace_back( std::move( c ) );
    }

    [[nodiscard]] uint32_t inactive() const { return available.size(); }
    [[nodiscard]] uint32_t active() const { return total; }

  private:
    Factory creator;
    std::deque<Ptr> available;
    std::mutex mutex;
    uint32_t total = 0;
    uint32_t initialSize;
    uint32_t maxSize;
  };
}
