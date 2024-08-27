//
// Created by Rakesh on 02/06/2020.
//

#include "configuration.hpp"
#include "../../common/util/json.hpp"

std::string spt::model::Configuration::str() const
{
  return util::json::str( *this );
}
