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

bool spt::model::Document::valid() const
{
  if (!view) return false;

  auto find = [this]( std::string_view key )
  {
    auto it = view->find( key );
    return it != view->end();
  };

  return find( "action" ) && find( "database" ) &&
    find( "collection" ) && find( "document" );
}
