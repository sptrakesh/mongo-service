//
// Created by Rakesh on 2019-05-16.
//

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "../../src/log/NanoLog.h"

int main( int argc, char* argv[] )
{
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-test", true );
  return Catch::Session().run( argc, argv );
}

