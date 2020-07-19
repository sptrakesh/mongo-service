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
