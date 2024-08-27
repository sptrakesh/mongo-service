//
// Created by Rakesh on 22/02/2022.
//

#pragma once

#include "contextholder.hpp"
#include "request.hpp"
#include "pool/pool.hpp"

#include <tuple>
#include <boost/asio/awaitable.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>

namespace spt::mongoservice::api
{
  /**
   * Invoke once before using API.  Initialises connection pool to the service.
   *
   * @param server The hostname of the server to connect to.
   * @param port The TCP port to connect to.
   * @param application The name of the client application to use when sending
   *   command to service.
   * @param ioc The optional io context to use for the connection.
   */
  void init( std::string_view server, std::string_view port,
      std::string_view application = {},
      const pool::Configuration& poolConfiguration = pool::Configuration{},
      boost::asio::io_context& ioc = ContextHolder::instance().ioc );

  enum class ResultType : std::uint_fast8_t {
    /**
     * Command executed against server successfully.  This does not indicate
     * the actual command being executed succeeded, just that the command was
     * executed by the service, and a response received.
     */
    success,
    /**
     * Command could not be executed against the service due to failure retrieving
     * a connection from the pool.
     */
    poolFailure,
    /**
     * Command was sent to the service, but no response was received.
     */
    commandFailure };
  using Response = std::tuple<ResultType, std::optional<bsoncxx::document::value>>;

  /**
   * Execute the command specified in the input document view against the
   * service.
   * @param document The document with the command to execute.
   * @param bufSize Optional initial size of buffer to use to receive data.
   * @return The result document or std::nullopt if invalid data was received.
   *   Caller must check the document contents to ensure successful execution
   *   of the command.
   */
  Response execute( bsoncxx::document::view document, std::size_t bufSize = 4 * 1024 );

  /**
   * Execute the command encapsulated in the request against the service.
   * @param req The request model to use to build the command.
   * @param bufSize Optional initial size of buffer to use to receive data.
   * @return The result document or std::nullopt if invalid data was received.
   *   Caller must check the document contents to ensure successful execution
   *   of the command.
   */
  Response execute( const Request& req, std::size_t bufSize = 4 * 1024 );

  using AsyncResponse = boost::asio::awaitable<Response>;
  /**
   * Execute the command specified in the input document view against the
   * service asynchronously.
   * @param document The document with the command to execute.
   * @return The result document or std::nullopt if invalid data was received.
   *   Caller must check the document contents to ensure successful execution
   *   of the command.
   */
  AsyncResponse executeAsync( bsoncxx::document::view document );

  /**
   * Execute the command encapsulated in the request against the service asynchronously.
   * @param req The request model to use to build the command.
   * @return The result document or std::nullopt if invalid data was received.
   *   Caller must check the document contents to ensure successful execution
   *   of the command.
   */
  AsyncResponse executeAsync( Request req );
}