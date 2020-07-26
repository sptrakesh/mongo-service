//
// Created by Rakesh on 20/07/2020.
//

#pragma once
#include "model/document.h"

#include <bsoncxx/document/view_or_value.hpp>

namespace spt::db
{
  bsoncxx::document::view_or_value process( const spt::model::Document& document );
}
