//
// Created by Rakesh on 18/07/2020.
//

#include "server.h"
#include "session.h"
#include "log/NanoLog.h"
#include "model/configuration.h"

using spt::server::Server;
using tcp = boost::asio::ip::tcp;

Server::Server( boost::asio::io_context& ioc ) :
  acceptor{ ioc, tcp::endpoint( tcp::v4(),
      static_cast<short>( model::Configuration::instance().port ) ) }
{
#ifdef __APPLE__
  acceptor.set_option( tcp::acceptor::reuse_address( true ) );
#endif
  doAccept();
}

void Server::doAccept()
{
  acceptor.async_accept(
      [this]( boost::system::error_code ec, tcp::socket socket )
      {
        if ( !ec )
        {
          LOG_DEBUG << "Accepting connection from " << socket.remote_endpoint().address().to_string();
          std::make_shared<Session>( std::move(socket) )->start();
        }
        else
        {
          LOG_WARN << "Error accepting connection " << ec.message();
        }

        doAccept();
      });
}
