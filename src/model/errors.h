//
// Created by Rakesh on 19/07/2020.
//

#pragma once

#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>

namespace spt::model
{
  bsoncxx::document::view notBson();
  bsoncxx::document::view missingField();
  bsoncxx::document::view invalidAction();
  bsoncxx::document::view missingId();
  bsoncxx::document::view insertError();
  bsoncxx::document::view invalidAUpdate();
  bsoncxx::document::view updateError();
  bsoncxx::document::view unexpectedError();
  bsoncxx::document::view createVersionFailed();
  bsoncxx::document::view notFound();
  bsoncxx::document::value withMessage( std::string_view message );
}
