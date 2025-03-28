//
// Created by Rakesh on 10/03/2025.
//

#pragma once

#include <functional>

namespace spt::util
{
  class ScopeGuard
  {
  public:
    template<class Callable>
    ScopeGuard( Callable&& fn ) : fn_( std::forward<Callable>( fn ) ) {}

    ScopeGuard( ScopeGuard&& other ) : fn_( std::move( other.fn_ ) )
    {
      other.fn_ = nullptr;
    }

    ~ScopeGuard()
    {
      // must not throw
      if (fn_) fn_();
    }

    ScopeGuard(const ScopeGuard &) = delete;
    void operator=(const ScopeGuard &) = delete;

  private:
    std::function<void()> fn_;
  };
}

#define DEFER_CONCAT_(a, b) a ## b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_(a,b)
#define DEFER(fn) spt::util::ScopeGuard DEFER_CONCAT(__defer__, __LINE__) = [&] ( ) { fn ; }