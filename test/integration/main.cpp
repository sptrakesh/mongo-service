//
// Created by Rakesh on 2019-05-16.
//

#include "../../src/log/NanoLog.hpp"
#include "../../src/api/api.hpp"

#include <catch2/catch_session.hpp>
#if defined(_WIN32) || defined(WIN32)
#include <filesystem>
#endif

int main( int argc, char* argv[] )
{
#if defined(_WIN32) || defined(WIN32)
  auto str = std::filesystem::temp_directory_path().string();
  str.append( "\\" );
  nanolog::initialize( nanolog::GuaranteedLogger(), str, "mongo-service-itest", true );
#else
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-itest", true );
#endif
  spt::mongoservice::api::init( "localhost", "2020", "integration-test" );
  return Catch::Session().run( argc, argv );
}

