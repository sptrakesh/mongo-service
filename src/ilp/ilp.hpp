//
// Created by Rakesh on 2019-05-24.
//
// https://github.com/bjlaub/simpletsdb-client

#pragma once

#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#if defined __has_include
#if __has_include("../log/NanoLog.hpp")
#include "../log/NanoLog.hpp"
#else
#include <log/NanoLog.h>
#endif
#endif

namespace spt::ilp
{
  /**
   * A TCP ILP client.  Use to write data using ILP protocol to a TSDB.
   */
  struct ILPClient
  {
    /**
     *
     * @param context The `io_context` instance to use to handle IO.
     * @param host The hostname of the ILP server.
     * @param port The port on which the ILP server listens.
     */
    ILPClient( boost::asio::io_context& context, std::string_view host, std::string_view port );

    /**
     *
     * @param ilp The data to send over ILP.  Generally no errors are returned or exceptions thrown.  Errors are logged.
     */
    void write( std::string&& ilp );

    ILPClient( const ILPClient& ) = delete;
    ILPClient& operator=( const ILPClient& ) = delete;

    ~ILPClient()
    {
      boost::system::error_code ec;
      auto _ = s.shutdown( boost::asio::socket_base::shutdown_both, ec );
      if ( ec ) LOG_CRIT << "Error shutting down socket. " << ec.message();
      _ = s.close( ec );
      if ( ec ) LOG_CRIT << "Error closing socket. " << ec.message();
    }

  private:
    boost::asio::ip::tcp::socket& socket();

    boost::asio::ip::tcp::socket s;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    boost::asio::streambuf buffer;
    std::string host;
    std::string port;
    std::chrono::time_point<std::chrono::system_clock> last{ std::chrono::system_clock::now() };
  };
}
