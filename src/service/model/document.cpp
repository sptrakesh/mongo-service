//
// Created by Rakesh on 19/07/2020.
//

#include "document.hpp"
#include "../log/NanoLog.hpp"
#include "../common/util/bson.hpp"
#include "../common/magic_enum/magic_enum.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/validate.hpp>

using spt::model::Document;

Document::Document( const uint8_t* buffer, std::size_t length ) :
    view{ bsoncxx::validate( buffer, length ) } {}

Document::Document( bsoncxx::document::view view ) : view{ view } {}

std::optional<bsoncxx::document::view> Document::bson() const
{
  return view;
}

bool Document::valid() const
{
  enum class Action : uint8_t { create, retrieve, update, count,
    index, dropIndex,
    bulk, pipeline, transaction,
    createCollection, renameCollection, dropCollection,
    createTimeseries };
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

  if ( *action != "delete" )
  {
    if ( const auto e = magic_enum::enum_cast<Action>( *action ); !e )
    {
      LOG_DEBUG << "Invalid action " << *action;
      return false;
    }
  }

  return action == "transaction" ? find( "document" ) :
    find( "action" ) && find( "database" ) &&
      find( "collection" ) && find( "document" );
}

std::string Document::action() const
{
  return util::bsonValue<std::string>( "action", *view );
}

std::string Document::database() const
{
  return util::bsonValue<std::string>( "database", *view );
}

std::string Document::collection() const
{
  return util::bsonValue<std::string>( "collection", *view );
}

bsoncxx::document::view Document::document() const
{
  return util::bsonValue<bsoncxx::document::view>( "document", *view );
}

std::string Document::json() const
{
  return bsoncxx::to_json( *view );
}

std::optional<bsoncxx::document::view> Document::options() const
{
  return util::bsonValueIfExists<bsoncxx::document::view>( "options", *view );
}

std::optional<bsoncxx::document::view> Document::metadata() const
{
  return util::bsonValueIfExists<bsoncxx::document::view>( "metadata", *view );
}

std::optional<std::string> Document::correlationId() const
{
  auto v = util::toString( "correlationId", *view );
  return v.empty() ? std::nullopt : std::optional<std::string>{ v };
}

std::optional<std::string> Document::application() const
{
  return util::bsonValueIfExists<std::string>( "application", *view );
}

std::optional<bool> Document::skipVersion() const
{
  return util::bsonValueIfExists<bool>( "skipVersion", *view );
}

std::optional<bool> Document::skipMetric() const
{
  return util::bsonValueIfExists<bool>( "skipMetric", *view );
}
