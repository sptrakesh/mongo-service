//
// Created by Rakesh on 2019-05-14.
//

#include "config.h"

#include <sstream>

using spt::util::Configuration;

std::string Configuration::str() const
{
  std::ostringstream ss;
  ss << "{\"port\":" << port <<
    ", \"threads\":" << threads <<
    R"(, "file": ")" << file <<
    "\"}";
  return ss.str();
}
