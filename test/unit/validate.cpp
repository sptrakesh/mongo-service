//
// Created by Rakesh on 22/02/2024.
//

#include "model.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <bsoncxx/builder/list.hpp>

using namespace spt::util;

SCENARIO( "Content scanning test suite", "[scan]" )
{
  GIVEN( "String values with potentially harmful data" )
  {
    WHEN( "Scanning string with script tag" )
    {
      const auto data = "Test <script>alert('Last Name');</script>"s;
      CHECK( spt::util::json::hasDangerousContent( data ) );
    }

    AND_WHEN( "Scanning string with just function invocation" )
    {
      const auto data = "Test alert('Last Name');"s;
      CHECK_FALSE( spt::util::json::hasDangerousContent( data ) );
    }

    AND_WHEN( "Scanning string with just function invocation and assignment" )
    {
      const auto data = "Test name = alert('Last Name');"s;
      CHECK( spt::util::json::hasDangerousContent( data ) );
    }

    AND_WHEN( "Scanning string with variable assignment" )
    {
      const auto data = "Test let name = retrieve('Last Name');"s;
      CHECK( spt::util::json::hasDangerousContent( data ) );
    }
  }
}

SCENARIO( "Invalid data test suite", "[invalid]" )
{
  GIVEN( "Invalid input data" )
  {
    WHEN( "Validating BSON array with invalid data" )
    {
      auto arr = bsoncxx::builder::list{
          {
              { "specialChars", "$@%^&*(!@$)123(^@#abc" },
              { "valid", "simple string" }
          },
          {
              { "int", 12 },
              { "double", 24.5 },
              { "bool", false }
          }
      };
      CHECK_FALSE( spt::util::json::validate( "test", arr.view().get_array().value ) );
    }

    AND_WHEN( "Validating BSON document with invalid data" )
    {
      auto obj = bsoncxx::builder::list{
          "string", "a string value",
          "int", 12,
          "double", 24.5,
          "bool", false,
          "specialChars", "$@%^&*123(!@$)(^@#abc"
      };
      CHECK_FALSE( spt::util::json::validate( "test", obj.view().get_document().value ) );
    }

    AND_WHEN( "Validating JSON array with invalid data" )
    {
      auto arr = boost::json::array{
          boost::json::object{
              { "int", 12 },
              { "double", 24.5 },
              { "bool", false }
            },
          boost::json::object{
              { "specialChars", "123$@%^&*(!@$)(^@#abc" },
              { "valid", "simple string" }
            }
      };
      CHECK_FALSE( spt::util::json::validate( "test", arr ) );
    }

    AND_WHEN( "Validating JSON object with invalid data" )
    {
      auto obj = boost::json::object{
          { "int", 12 },
          { "double", 24.5 },
          { "bool", false },
          { "specialChars", "$@%^&*(!@$)(^@#abc123" },
          { "valid", "simple string" }
      };
      CHECK_FALSE( spt::util::json::validate( "test", obj ) );
    }
  }
}