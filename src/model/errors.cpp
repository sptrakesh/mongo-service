//
// Created by Rakesh on 19/07/2020.
//

#include "errors.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>

bsoncxx::document::view spt::model::notBson()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Payload not BSON") );
  return document.view();
}

bsoncxx::document::view spt::model::missingField()
{
  namespace basic = bsoncxx::builder::basic;
  using bsoncxx::document::value;
  using basic::kvp;
  using basic::make_document;
  using basic::make_array;
  static value document = make_document(
      kvp("error", "Missing mandatory field(s)."),
      kvp( "fields", make_array( "action", "database", "collection", "document" ) ) );
  return document.view();
}

bsoncxx::document::view spt::model::invalidAction()
{
  namespace basic = bsoncxx::builder::basic;
  using bsoncxx::document::value;
  using basic::kvp;
  using basic::make_document;
  using basic::make_array;
  static value document = make_document(
      kvp("error", "Invalid action."),
      kvp( "valid", make_array( "create", "retrieve", "update", "delete" ) ) );
  return document.view();
}

bsoncxx::document::view spt::model::missingId()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Document id not specified") );
  return document.view();
}

bsoncxx::document::view spt::model::insertError()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Error inserting document.") );
  return document.view();
}

bsoncxx::document::view spt::model::invalidAUpdate()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Invalid update document.") );
  return document.view();
}

bsoncxx::document::view spt::model::updateError()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Error updating document.") );
  return document.view();
}

bsoncxx::document::view spt::model::notModifyable()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Cannot edit version history.") );
  return document.view();
}

bsoncxx::document::view spt::model::createVersionFailed()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Error creating version history document.") );
  return document.view();
}

bsoncxx::document::view spt::model::notFound()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Not found.") );
  return document.view();
}

bsoncxx::document::view spt::model::unexpectedError()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Unexpected error encountered.") );
  return document.view();
}

bsoncxx::document::view spt::model::poolExhausted()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Pool exhausted.") );
  return document.view();
}

bsoncxx::document::value spt::model::withMessage( std::string_view message )
{
  using bsoncxx::builder::basic::kvp;
  return bsoncxx::builder::basic::make_document( kvp("error", message) );
}
