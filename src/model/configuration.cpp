//
// Created by Rakesh on 02/06/2020.
//

#include "configuration.h"
#include <sstream>

std::string spt::model::Configuration::str() const
{
  std::ostringstream ss;
  ss << R"({"port": )" << port <<
     R"(, "threads": )" << threads <<
     R"(, "mongo": {"database": ")" << versionHistoryDatabase <<
     R"(", "historyCollection": ")" << versionHistoryCollection <<
     R"(", "metricsCollection": ")" << metrics <<
     "\"}}";
  return ss.str();
}
