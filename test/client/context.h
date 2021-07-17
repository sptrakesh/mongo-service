//
// Created by Rakesh on 17/07/2021.
//

#include <boost/asio/io_context.hpp>
#include <thread>

namespace spt::client
{
  struct Context
  {
    static Context& instance()
    {
      static Context ctx;
      return ctx;
    }

    ~Context() = default;
    Context(Context&) = delete;
    Context& operator=(Context&) = delete;

    boost::asio::io_context ioc;

  private:
    Context() : ioc{ static_cast<int>(std::thread::hardware_concurrency() ) } {}
  };
}
