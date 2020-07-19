//
// Created by Rakesh on 19/07/2020.
//

#include "document.h"

#include <bsoncxx/validate.hpp>

using spt::model::Document;

Document::Document( const boost::asio::streambuf& buffer, std::size_t length )
{
  view = bsoncxx::validate( reinterpret_cast<const uint8_t*>( buffer.data().data() ), length );
}

std::optional<bsoncxx::document::view> Document::bson() const
{
  return view;
}
