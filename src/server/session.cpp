//
// Created by Rakesh on 18/07/2020.
//

#include "session.h"
#include "log/NanoLog.h"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

using spt::server::Session;
using tcp = boost::asio::ip::tcp;

Session::Session( tcp::socket socket ) : socket{ std::move( socket ) }
{
  data.reserve( 8192 );
  data.push_back(' '); // If empty async_read does not insert any data
}

void Session::start()
{
  doRead();
}

void Session::doRead()
{
  auto constexpr maxBytes = 8192 * 8192;

  auto self{ shared_from_this() };
  boost::asio::async_read( socket, boost::asio::buffer( data, maxBytes ),
      [this, self]( boost::system::error_code ec, std::size_t length )
      {
        if ( !ec )
        {
          doWrite( length );
        }
      });
}

void Session::doWrite( std::size_t length )
{
  auto self{ shared_from_this() };
  boost::asio::async_write(socket, boost::asio::buffer( data, length ),
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (!ec)
        {
          doRead();
        }
        else
        {
          LOG_DEBUG << "Error writing data " << ec.message();
        }
      });
}
