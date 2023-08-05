//
// Created by Rakesh on 05/08/2023.
//

#include "internal.h"
#include "../common/util/bson.h"

mongocxx::write_concern spt::db::internal::writeConcern( bsoncxx::document::view view )
{
  using spt::util::bsonValueIfExists;

  auto w = mongocxx::write_concern{};

  if ( auto journal = bsonValueIfExists<bool>( "journal", view ); journal ) w.journal( *journal );
  if ( auto nodes = bsonValueIfExists<int32_t>( "nodes", view ); nodes ) w.nodes( *nodes );

  if ( auto level = bsonValueIfExists<int32_t>( "acknowledgeLevel", view ); level ) w.acknowledge_level( static_cast<mongocxx::write_concern::level>( *level ) );
  else w.acknowledge_level( mongocxx::write_concern::level::k_majority );

  if ( auto majority = bsonValueIfExists<std::chrono::milliseconds>( "majority", view ); majority ) w.majority( *majority );
  if ( auto tag = bsonValueIfExists<std::string>( "tag", view ); tag ) w.tag( *tag );
  if ( auto timeout = bsonValueIfExists<std::chrono::milliseconds>( "timeout", view ); timeout ) w.timeout( *timeout );

  return w;
}
