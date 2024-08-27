//
// Created by Rakesh on 01/10/2019.
//

#include "logger.hpp"
#include "../log/NanoLog.hpp"

void spt::db::Logger::operator()( mongocxx::log_level level,
    bsoncxx::stdx::string_view domain,
    bsoncxx::stdx::string_view message ) noexcept
{
  switch (level)
  {
  case mongocxx::log_level::k_critical:
  case mongocxx::log_level::k_error:
    LOG_CRIT << "domain: " << std::string{ domain.data(), domain.size() }
             << "\nmessage: " << std::string{ message.data(), message.size() };
    break;
  case mongocxx::log_level::k_warning:
    LOG_WARN << "domain: " << std::string{ domain.data(), domain.size() }
             << "\nmessage: " << std::string{ message.data(), message.size() };
    break;
  default:
    LOG_INFO << "domain: " << std::string{ domain.data(), domain.size() }
             << "\nmessage: " << std::string{ message.data(), message.size() };
  }
}
