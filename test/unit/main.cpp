//
// Created by Rakesh on 2019-05-16.
//

#include "../../src/log/NanoLog.h"
#include <catch2/catch_session.hpp>

int main( int argc, char* argv[] )
{
  nanolog::initialize( nanolog::GuaranteedLogger(), "/tmp/", "mongo-service-test", false );
  return Catch::Session().run( argc, argv );
}
