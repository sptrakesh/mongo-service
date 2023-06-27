//
// Created by Rakesh on 02/06/2020.
//

#include "configuration.h"
#include "../../common/util/json.h"

std::string spt::model::Configuration::str() const
{
  return util::json::str( *this );
}
