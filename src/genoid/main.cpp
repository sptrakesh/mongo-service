//
// Created by Rakesh on 25/10/2024.
//

#include "../common/util/bson.hpp"
#include "../common/util/clara.hpp"
#include "../common/util/date.hpp"
#include "../log/NanoLog.hpp"

#include <cstdlib>
#include <iostream>

int main( int argc, char const * const * argv )
{
  using clara::Opt;

  std::string at;
  std::string logLevel{ "info" };
  std::string dir{ "/tmp/" };
  bool help = false;

  auto options = clara::Help( help ) |
      Opt( at, "2024-10-25T14:30:30.000Z" )["-a"]["--at"]( "Generate BSON ObjectId at specified timestamp." ) |
      Opt( logLevel, "info" )["-l"]["--log-level"]( "Log level to use [debug|info|warn|critical] (default info)." ) |
      Opt( dir, "/tmp/" )["-o"]["--log-dir"]("Log directory (default /tmp/)");

  auto result = options.parse( clara::Args( argc, argv ));
  if ( !result )
  {
    std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
    exit( 1 );
  }

  if ( help )
  {
    options.writeToStream( std::cout );
    exit( 0 );
  }

  if ( logLevel == "debug" ) nanolog::set_log_level( nanolog::LogLevel::DEBUG );
  else if ( logLevel == "info" ) nanolog::set_log_level( nanolog::LogLevel::INFO );
  else if ( logLevel == "warn" ) nanolog::set_log_level( nanolog::LogLevel::WARN );
  else if ( logLevel == "critical" ) nanolog::set_log_level( nanolog::LogLevel::CRIT );
  nanolog::initialize( nanolog::GuaranteedLogger(), dir, "genoid", false );

  if ( at.empty() )
  {
    std::cout << bsoncxx::oid{}.to_string() << '\n';
    exit( 0 );
  }

  auto dt = spt::util::parseISO8601( at );
  if ( !dt.has_value() )
  {
    std::cerr << "Error parsing date-time value.  Date time must be specified in ISO8601 format (yyyy-MM-dd'T'HH:mm:ss.SSSZ\n";
    exit( 1 );
  }

  auto id = spt::util::generateId( dt.value(), bsoncxx::oid{} );
  LOG_INFO << "ObjectId at " << at << ": " << id;
  std::cout << id.to_string() << '\n';
  exit( 0 );
}
