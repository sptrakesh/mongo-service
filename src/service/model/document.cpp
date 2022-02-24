//
// Created by Rakesh on 19/07/2020.
//

#include "document.h"
#include "../log/NanoLog.h"
#include "../common/util/bson.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>

using spt::model::Document;

Document::Document( const uint8_t* buffer, std::size_t length ) :
    view{ bsoncxx::validate( buffer, length ) }
{
}

Document::Document( bsoncxx::document::view view ) : view{ view } {}

std::optional<bsoncxx::document::view> Document::bson() const
{
  return view;
}

bool spt::model::Document::valid() const
{
  if ( !view ) return false;

  auto find = [this]( std::string_view key )
  {
    auto it = view->find( key );
    auto result = it != view->end();
    if ( !result ) LOG_DEBUG << "Document does not have required property: " << key;
    return result;
  };

  const auto action = util::bsonValueIfExists<std::string>( "action", *view );
  if ( !action )
  {
    LOG_DEBUG << "Document does not have action property";
    return false;
  }

  return action == "transaction" ? find( "document" ) :
    find( "action" ) && find( "database" ) &&
      find( "collection" ) && find( "document" );
}

std::string spt::model::Document::action() const
{
  return util::bsonValue<std::string>( "action", *view );
}

std::string spt::model::Document::database() const
{
  return util::bsonValue<std::string>( "database", *view );
}

std::string spt::model::Document::collection() const
{
  return util::bsonValue<std::string>( "collection", *view );
}

bsoncxx::document::view spt::model::Document::document() const
{
  return util::bsonValue<bsoncxx::document::view>( "document", *view );
}

std::string spt::model::Document::json() const
{
  return bsoncxx::to_json( *view );
}

std::optional<bsoncxx::document::view> spt::model::Document::options() const
{
  return util::bsonValueIfExists<bsoncxx::document::view>( "options", *view );
}

std::optional<bsoncxx::document::view> spt::model::Document::metadata() const
{
  return util::bsonValueIfExists<bsoncxx::document::view>( "metadata", *view );
}

std::optional<std::string> spt::model::Document::correlationId() const
{
  auto v = util::toString( "correlationId", *view );
  return v.empty() ? std::nullopt : std::optional<std::string>{ v };
}

std::optional<std::string> spt::model::Document::application() const
{
  return util::bsonValueIfExists<std::string>( "application", *view );
}

std::optional<bool> spt::model::Document::skipVersion() const
{
  return util::bsonValueIfExists<bool>( "skipVersion", *view );
}