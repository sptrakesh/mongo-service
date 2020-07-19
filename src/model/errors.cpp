//
// Created by Rakesh on 19/07/2020.
//

#include "errors.h"

#include <bsoncxx/builder/basic/document.hpp>

bsoncxx::document::view spt::model::notBson()
{
  using bsoncxx::document::value;
  using bsoncxx::builder::basic::kvp;
  static value document = bsoncxx::builder::basic::make_document( kvp("error", "Payload not BSON") );
  return document.view();
}
