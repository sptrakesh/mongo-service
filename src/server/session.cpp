//
// Created by Rakesh on 18/07/2020.
//

#include "session.h"
#include "log/NanoLog.h"
#include "model/document.h"
#include "model/errors.h"

#include <boost/asio/write.hpp>

using spt::server::Session;
using tcp = boost::asio::ip::tcp;

Session::Session( tcp::socket socket ) : socket{ std::move( socket ) } {}

void Session::start()
{
  doRead();
}

void Session::doRead()
{
  constexpr auto maxBytes = 128 * 1024;
  auto self{ shared_from_this() };
  buffer.consume( buffer.size() );

  socket.async_receive( buffer.prepare( maxBytes ),
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
  buffer.commit( length );

  const auto doc = model::Document{ buffer, length };
  if ( !doc.bson() )
  {
    buffer.consume( buffer.size() );
    std::ostream os{ &buffer };
    auto view = spt::model::notBson();
    os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    LOG_DEBUG << "Invalid bson received.  Returning not bson message...";
  }

  if ( !doc.valid() )
  {
    buffer.consume( buffer.size() );
    std::ostream os{ &buffer };
    auto view = spt::model::missingField();
    os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    LOG_DEBUG << "Invalid bson received.  Returning not bson message...";
  }

  boost::asio::async_write( socket,
      buffer,
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        //document.data.clear();
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
