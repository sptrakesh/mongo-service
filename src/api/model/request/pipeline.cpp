//
// Created by Rakesh on 25/12/2024.
//

#include "pipeline.hpp"
#include "../../../log/NanoLog.hpp"

void spt::mongoservice::api::model::request::populate( const Pipeline::Document::Stage& model, bsoncxx::builder::stream::document& builder )
{
  if ( !model.value ) return;
  builder << model.command << *model.value;
}

void spt::mongoservice::api::model::request::populate( const Pipeline::Document::Stage& model, boost::json::object& object )
{
  using bsoncxx::type;

  if ( !model.value ) return;
  switch ( model.value->view().type() )
  {
  case type::k_array:
    object.emplace( model.command, util::toJson( model.value->view().get_array().value ) );
    break;
  case type::k_document:
    object.emplace( model.command, util::toJson( model.value->view().get_document().value ) );
    break;
  case type::k_oid:
    object.emplace( model.command, model.value->view().get_oid().value.to_string() );
    break;
  case type::k_bool:
    object.emplace( model.command, model.value->view().get_bool().value );
    break;
  case type::k_int32:
    object.emplace( model.command, model.value->view().get_int32().value );
    break;
  case type::k_int64:
    object.emplace( model.command, model.value->view().get_int64().value );
    break;
  case type::k_double:
    object.emplace( model.command, model.value->view().get_double().value );
    break;
  case type::k_string:
    object.emplace( model.command, model.value->view().get_string().value );
    break;
  case type::k_date:
    object.emplace( model.command, util::isoDateMillis( model.value->view().get_date().value ) );
    break;
  default:
    LOG_WARN << "Unsupported type: " << bsoncxx::to_string( model.value->view().type() );
  }
}
