//
// Created by Rakesh on 05/09/2024.
//

#include <bsoncxx/oid.hpp>

#include "../../src/log/NanoLog.hpp"

#include <catch2/catch_test_macros.hpp>

SCENARIO( "NanoLog test suite", "[nanolog]" )
{
  GIVEN( "Property initialised logging system" )
  {
    WHEN( "NanoLog logging int16_t" )
    {
      LOG_DEBUG << "int16_t value: " << int16_t{ 12 };
    }

    AND_WHEN( "NanoLog logging uint16_t" )
    {
      LOG_DEBUG << "uint16_t value: " << uint16_t{ 12 };
    }

    AND_WHEN( "NanoLog logging float" )
    {
      LOG_DEBUG << "float value: " << float{ 12.5 };
    }

    AND_WHEN( "NanoLog logging BSON object id" )
    {
      LOG_DEBUG << "BSON object id value: " << bsoncxx::oid{};
    }
  }
}
