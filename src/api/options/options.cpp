//
// Created by Rakesh on 20/12/2024.
//

#include "readpreference.hpp"
#include "writeconcern.hpp"
#include "../../log/NanoLog.hpp"
#include "../../common/util/bson.hpp"

#include <utility>

void spt::mongoservice::api::options::populate( ReadPreference& model, bsoncxx::document::view bson )
{
  if ( auto level = util::bsonValueIfExists<int32_t>( "mode", bson ); level )
  {
    model.mode = static_cast<ReadPreference::ReadMode>( static_cast<uint8_t>( *level ) );
  }
}

void spt::mongoservice::api::options::populate( const ReadPreference& model, bsoncxx::builder::stream::document& builder )
{
  builder << "mode" << static_cast<int32_t>( std::to_underlying( model.mode ) );
}

void spt::mongoservice::api::options::populate( const ReadPreference& model, boost::json::object& object )
{
  object.emplace( "mode", static_cast<int32_t>( std::to_underlying( model.mode ) ) );
}

void spt::mongoservice::api::options::populate( WriteConcern& model, bsoncxx::document::view bson )
{
  if ( auto level = util::bsonValueIfExists<int32_t>( "acknowledgeLevel", bson ); level )
  {
    model.acknowledgeLevel = static_cast<WriteConcern::Level>( static_cast<uint8_t>( *level ) );
  }
}

void spt::mongoservice::api::options::populate( const WriteConcern& model, bsoncxx::builder::stream::document& builder )
{
  builder << "acknowledgeLevel" << static_cast<int32_t>( std::to_underlying( model.acknowledgeLevel ) );
}

void spt::mongoservice::api::options::populate( const WriteConcern& model, boost::json::object& object )
{
  object.emplace( "acknowledgeLevel", magic_enum::enum_name( model.acknowledgeLevel ) );
}
