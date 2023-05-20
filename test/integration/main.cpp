//
// Created by Rakesh on 2019-05-16.
//

#include "../../src/log/NanoLog.h"
#include "../../src/api/api.h"

#include <catch2/catch_session.hpp>

int main( int argc, char* argv[] )
{
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-itest", true );
  spt::mongoservice::api::init( "localhost", "2020", "integration-test" );
  return Catch::Session().run( argc, argv );
}

