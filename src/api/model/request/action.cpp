//
// Created by Rakesh on 20/12/2024.
//

#include "action.hpp"
#include "../../../common/magic_enum/magic_enum.hpp"
#include "../../../log/NanoLog.hpp"

using spt::mongoservice::api::model::request::Action;

bsoncxx::types::bson_value::value spt::util::bson( const Action& action )
{
  if ( action == Action::_delete ) return { "delete" };
  return { magic_enum::enum_name( action ) };
}

void spt::util::set( Action& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() != bsoncxx::type::k_string )
  {
    LOG_CRIT << "Invalid type for Role " << bsoncxx::to_string( value.type() );
    return;
  }

  if ( value.get_string().value == "delete" ) field = Action::_delete;
  else if ( auto s = magic_enum::enum_cast<Action>( value.get_string().value ); s ) field = *s;
  else LOG_WARN << "Invalid value for Action " << value.get_string().value;
}
