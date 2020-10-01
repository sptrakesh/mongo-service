//
// Created by Rakesh on 18/07/2020.
//

#include "session.h"
#include "db/storage.h"
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
  auto self{ shared_from_this() };
  buffer.consume( buffer.size() );
  readMore( std::move( self ) );
}

void spt::server::Session::readMore( std::shared_ptr<Session> session )
{
  constexpr auto maxBytes = 128 * 1024;
  socket.async_receive( buffer.prepare( maxBytes ),
      [this, session]( boost::system::error_code ec, std::size_t length )
      {
        if ( !ec )
        {
          doWrite( length );
        }
      });
}

void Session::doWrite( std::size_t length )
{
  constexpr auto maxBytes = 1024 * 1024;
  auto self{ shared_from_this() };

  const auto docSize = [this, length]()
  {
    if ( length < 5 ) return length;

    const auto data = reinterpret_cast<const uint8_t*>( buffer.data().data() );
    uint32_t len;
    memcpy( &len, data, sizeof(len) );
    return std::size_t( len );
  };

  buffer.commit( length );
  totalRead += length;
  if ( documentSize == 0 ) documentSize = docSize();
  if ( documentSize < maxBytes && documentSize != totalRead ) return readMore( std::move( self ) );

  const auto doc = model::Document{ buffer, totalRead };
  buffer.consume( buffer.size() );
  documentSize = 0;
  totalRead = 0;
  std::ostream os{ &buffer };

  if ( !doc.bson() )
  {
    auto view = spt::model::notBson();
    os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    LOG_DEBUG << "Invalid bson received. Length: " << int(length) << ". Returning not bson message...";
  }
  else if ( !doc.valid() )
  {
    auto view = spt::model::missingField();
    os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    LOG_DEBUG << "Invalid bson received.  Returning not bson message...";
  }
  else
  {
    try
    {
      auto v = db::process( doc );
      auto view = v.view();
      os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error processing request " << ex.what();
      auto view = model::unexpectedError();
      os.write( reinterpret_cast<const char*>( view.data() ), view.length() );
    }
  }

  boost::asio::async_write( socket,
      buffer,
      [this, self](boost::system::error_code ec, std::size_t length)
      {
        LOG_DEBUG << "Wrote bytes: " << int(length);
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
