//
// Created by Rakesh on 08/04/2026.
//

#include "model.hpp"
#include "../../src/common/util/graphql.hpp"

#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace spt::util;

SCENARIO( "GraphQL serialisation test suite", "[graphql]" )
{
  GIVEN( "A fully visitable struct" )
  {
    auto obj = test::serial::Full{};
    obj.identifier = "test";
    obj.notVisitable.identifier = "xyz-987";
    obj.notVisitable.integer = 456;
    obj.customFields.id = "lmn-456";
    obj.identifier = "abc-123"s;
    obj.nested = test::serial::Full::Nested{ .identifier = "nested-123"s, .integer = 1234, .number = 1.234, .date = std::chrono::system_clock::now(), .numbers = { 1.2, 2.3, 3.4 }, .level = test::serial::Full::Level::Info };
    obj.nesteds = {
      test::serial::Full::Nested{ .identifier = "nested-1"s, .integer = 1, .number = 1.1, .date = std::chrono::system_clock::now(), .numbers = { 1.1, 1.2, 1.3 } },
      test::serial::Full::Nested{ .identifier = "nested-2"s, .integer = 2, .number = 2.1, .date = std::chrono::system_clock::now(), .numbers = { 2.1, 2.2, 2.3 } },
      test::serial::Full::Nested{ .identifier = "nested-3"s, .integer = 3, .number = 3.1, .date = std::chrono::system_clock::now(), .numbers = { 3.1, 3.2, 3.3 } }
    };
    obj.nestedp = std::make_shared<test::serial::Full::Nested>();
    obj.nestedp->identifier = "nested-p"s;
    obj.nestedp->integer = 234;
    obj.nestedp->number = 234.567;
    obj.nestedp->date = std::chrono::system_clock::now();
    obj.nestedp->numbers = { 1.2, 2.3, 3.4 };
    obj.strings = { "one"s, "two"s, "three"s };
    obj.ostring = "some string value"s;
    obj.obool = true;
    obj.time = std::chrono::system_clock::now();
    obj.boolean = true;

    WHEN( "Serialising the struct to query" )
    {
      const auto query = graphql::query( obj );
      INFO( query );
      CHECK_FALSE( query.empty() );
      CHECK( query.starts_with( "{" ) );
      CHECK( query.ends_with( "}\n" ) );
      CHECK( query.contains( "notVisitable { identifier integer }" ) );
      CHECK( query.contains( "identifier integer number date numbers level" ) );
      CHECK( query.contains( "strings ostring obool time id boolean" ) );
    }

    AND_WHEN( "Serialising the struct to query with filter on notVisitable.identifier" )
    {
      const auto filter = boost::json::object{ { "notVisitable", boost::json::object{
        { "identifier", boost::json::value{} } }
      } };
      const auto query = graphql::query( obj, filter );
      INFO( query );
      CHECK_FALSE( query.empty() );
      CHECK( query.starts_with( "{" ) );
      CHECK( query.ends_with( "}\n" ) );
      CHECK( query.contains( "notVisitable { integer }" ) );
      CHECK( query.contains( "identifier integer number date numbers level" ) );
      CHECK( query.contains( "strings ostring obool time id boolean" ) );
    }

    AND_WHEN( "Serialising the struct to query with filter on notVisitable.integer" )
    {
      const auto filter = boost::json::object{ { "notVisitable", boost::json::object{
          { "integer", boost::json::value{} } }
      } };
      const auto query = graphql::query( obj, filter );
      INFO( query );
      CHECK_FALSE( query.empty() );
      CHECK( query.starts_with( "{" ) );
      CHECK( query.ends_with( "}\n" ) );
      CHECK( query.contains( "notVisitable { identifier }" ) );
      CHECK( query.contains( "identifier integer number date numbers level" ) );
      CHECK( query.contains( "strings ostring obool time id boolean" ) );
    }

    AND_WHEN( "Serialising the struct for mutation" )
    {
      const auto mut = graphql::mutation( obj );
      INFO( mut );
      CHECK_FALSE( mut.empty() );
      CHECK( mut.starts_with( "{" ) );
      CHECK( mut.ends_with( "}\n" ) );
      CHECK( mut.contains( R"(customFields: )" ) );
      CHECK( mut.contains( R"(notVisitable: { identifier: "xyz-987" integer: 456 })" ) );
      CHECK( mut.contains( R"(id: "lmn-456")" ) );
      CHECK( mut.contains( R"(identifier: "abc-123")" ) );
      CHECK( mut.contains( R"(nested: {)" ) );
      CHECK( mut.contains( R"(identifier: "nested-123")" ) );
      CHECK( mut.contains( R"(integer: 1234)" ) );
      CHECK( mut.contains( R"(number: 1.234)" ) );
      CHECK( mut.contains( R"(date: ")" ) );
      CHECK( mut.contains( R"(numbers: [)" ) );
      CHECK( mut.contains( R"(level: Info)" ) );
      CHECK( mut.contains( R"(nesteds: [)" ) );
      CHECK( mut.contains( R"(identifier: "nested-1")" ) );
      CHECK( mut.contains( R"(integer: 1)" ) );
      CHECK( mut.contains( R"(number: 1.1)" ) );
      CHECK( mut.contains( R"(identifier: "nested-2")" ) );
      CHECK( mut.contains( R"(integer: 2)" ) );
      CHECK( mut.contains( R"(number: 2.1)" ) );
      CHECK( mut.contains( R"(identifier: "nested-3")" ) );
      CHECK( mut.contains( R"(integer: 3)" ) );
      CHECK( mut.contains( R"(number: 3.1)" ) );
      CHECK( mut.contains( R"(nestedp: {)" ) );
      CHECK( mut.contains( R"(identifier: "nested-p")" ) );
      CHECK( mut.contains( R"(integer: 234)" ) );
      CHECK( mut.contains( R"(number: 234.567)" ) );
      CHECK( mut.contains( R"(strings: [)" ) );
      CHECK( mut.contains( R"("one")" ) );
      CHECK( mut.contains( R"("two")" ) );
      CHECK( mut.contains( R"("three")" ) );
      CHECK( mut.contains( R"(ostring: "some string value")" ) );
      CHECK( mut.contains( R"(boolean: true)" ) );
    }
  }
}
