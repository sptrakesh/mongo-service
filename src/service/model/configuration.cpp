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
     R"(", "logLevel": ")" << logLevel <<
     R"(, "mongo": {"versionHistory": {"database": ")" << versionHistoryDatabase <<
     R"(", "collection": ")" << versionHistoryCollection <<
     R"("}, "metrics": {"database": ")" << metricsDatabase <<
     R"(", "collection": ")" << metricsCollection <<
     "\"}}";
  return ss.str();
}
