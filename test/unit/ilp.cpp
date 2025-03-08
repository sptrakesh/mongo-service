//
// Created by Rakesh on 02/01/2023.
//

#include <catch2/catch_test_macros.hpp>
#include "../../src/ilp/builder.hpp"

using std::operator""sv;
using namespace std::literals::chrono_literals;

// https://questdb.io/docs/reference/api/ilp/overview/
SCENARIO( "ILP QuestDB Samples Test Suite" )
{
  GIVEN( "ILP Builder instance" )
  {
    auto builder = spt::ilp::Builder{};

    WHEN( "When building generic example" )
    {
      auto str = builder.
        startRecord( "readings"sv ).
          addTag( "city"sv, "London"sv ).
          addTag( "make"sv, "Omron"sv ).
          addValue( "temperature"sv, 23.5 ).
          addValue( "humidity"sv, 0.343 ).
          timestamp( 1465839830100400000ns ).
        endRecord().
        startRecord( "readings"sv ).
          addTag( "city"sv, "Bristol"sv ).
          addTag( "make"sv, "Honeywell"sv ).
          addValue( "temperature"sv, 23.2 ).
          addValue( "humidity"sv, 0.443 ).
          timestamp( 1465839830100600000ns ).
        endRecord().
        startRecord( "readings"sv ).
          addTag( "city"sv, "London"sv ).
          addTag( "make"sv, "Omron"sv ).
          addValue( "temperature"sv, 23.6 ).
          addValue( "humidity"sv, 0.348 ).
          timestamp( 1465839830100700000ns ).
        endRecord().
      finish();

      auto expected = R"(readings,city=London,make=Omron temperature=23.5,humidity=0.343 1465839830100400000
readings,city=Bristol,make=Honeywell temperature=23.2,humidity=0.443 1465839830100600000
readings,city=London,make=Omron temperature=23.6,humidity=0.348 1465839830100700000
)";

      REQUIRE( str == expected );
    }

    AND_WHEN( "Testing integer sample" )
    {
      auto str = builder.
          startRecord( "temps"sv ).
          addTag( "device"sv, "cpu"sv ).
          addTag( "location"sv, "south"sv ).
          addValue( "value"sv, 96 ).
          timestamp( 1638202821000000000ns ).
          endRecord().
          finish();

      auto expected = "temps,device=cpu,location=south value=96i 1638202821000000000\n";

      REQUIRE( str == expected );
    }

    AND_WHEN( "Testing string sample" )
    {
      auto str = builder.
          startRecord( "trade"sv ).
          addTag( "ticker"sv, "BTCUSD"sv ).
          addValue( "description"sv, std::string_view{ R"(this is a "rare" value)" } ).
          addValue( "user"sv, "John"sv ).
          timestamp( 1638202821000000000ns ).
          endRecord().
          finish();
      auto expected = R"(trade,ticker=BTCUSD description="this is a \"rare\" value",user="John" 1638202821000000000
)";
      CHECK( str == expected );
    }

    AND_WHEN( "Testing simple APMRecord" )
    {
      auto record = spt::ilp::APMRecord{ "abc123" };
      record.application = "unit test";
      record.tags.try_emplace( "key1", "value1" );
      record.tags.try_emplace( "key2", "value2" );
      record.values.try_emplace( "int", int64_t{ -1 } );
      record.values.try_emplace( "uint", uint64_t{ 1 } );
      record.values.try_emplace( "double", double{ 1.234 } );
      record.values.try_emplace( "string", std::string{ "string value" } );
      record.duration = std::chrono::nanoseconds{ 123 };

      auto str = builder.add( "apm", record ).finish();
      auto expected = std::format(
        "apm,application=unit\\ test,key1=value1,key2=value2 id=\"abc123\",duration=123i,double=1.234,int=-1i,string=\"string value\",uint=1u {}\n",
        std::chrono::duration_cast<std::chrono::nanoseconds>( record.timestamp.time_since_epoch() ).count() );
      CHECK( str == expected );
    }

    AND_WHEN( "Testing complex APMRecord" )
    {
      auto record = spt::ilp::APMRecord{ "abc123" };
      record.application = "unit test";
      record.tags.try_emplace( "key1", "value1" );
      record.tags.try_emplace( "key2", "value2" );
      record.values.try_emplace( "int", int64_t{ -1 } );
      record.values.try_emplace( "uint", uint64_t{ 1 } );
      record.values.try_emplace( "double", double{ 1.234 } );
      record.values.try_emplace( "string", std::string{ "string value" } );
      record.duration = std::chrono::nanoseconds{ 123 };

      record.processes.emplace_back( spt::ilp::APMRecord::Process::Type::Function );
      auto& p1 = record.processes.back();
      p1.duration = std::chrono::nanoseconds{ 256 };
      p1.tags.try_emplace( "key10", "value1" );
      p1.tags.try_emplace( "key11", "value2" );
      p1.values.try_emplace( "int", int64_t{ -10 } );
      p1.values.try_emplace( "uint", uint64_t{ 10 } );
      p1.values.try_emplace( "double", double{ 10.987 } );
      p1.values.try_emplace( "string", "first value" );

      record.processes.emplace_back( spt::ilp::APMRecord::Process::Type::Step );
      auto& p2 = record.processes.back();
      p2.duration = std::chrono::nanoseconds{ 5432 };
      p2.tags.try_emplace( "key10", "value10" );
      p2.tags.try_emplace( "key11", "value20" );
      p2.values.try_emplace( "int", int64_t{ -11 } );
      p2.values.try_emplace( "uint", uint64_t{ 11 } );
      p2.values.try_emplace( "double", double{ 11.987 } );
      p2.values.try_emplace( "string", "second value" );
      p2.values.try_emplace( "bool", false );

      record.processes.emplace_back( spt::ilp::APMRecord::Process::Type::Other );
      auto& p3 = record.processes.back();
      p3.duration = std::chrono::nanoseconds{ 932 };
      p3.tags.try_emplace( "key10", "value11" );
      p3.tags.try_emplace( "key11", "value21" );
      p3.values.try_emplace( "int", int64_t{ -12 } );
      p3.values.try_emplace( "uint", uint64_t{ 12 } );
      p3.values.try_emplace( "double", double{ 12.987 } );
      p3.values.try_emplace( "string", "third value" );
      p3.values.try_emplace( "bool", true );

      auto str = builder.add( "apm", record ).finish();
      auto expected = std::format( R"(apm,application=unit\ test,key1=value1,key2=value2 id="abc123",duration=123i,double=1.234,int=-1i,string="string value",uint=1u {}
apm,application=unit\ test,type=function,key10=value1,key11=value2 id="abc123",duration=256i,double=10.987,int=-10i,string="first value",uint=10u {}
apm,application=unit\ test,type=step,key10=value10,key11=value20 id="abc123",duration=5432i,bool=false,double=11.987,int=-11i,string="second value",uint=11u {}
apm,application=unit\ test,type=other,key10=value11,key11=value21 id="abc123",duration=932i,bool=true,double=12.987,int=-12i,string="third value",uint=12u {}
)",
        std::chrono::duration_cast<std::chrono::nanoseconds>( record.timestamp.time_since_epoch() ).count(),
        std::chrono::duration_cast<std::chrono::nanoseconds>( record.processes.front().timestamp.time_since_epoch() ).count(),
        std::chrono::duration_cast<std::chrono::nanoseconds>( record.processes[1].timestamp.time_since_epoch() ).count(),
        std::chrono::duration_cast<std::chrono::nanoseconds>( record.processes.back().timestamp.time_since_epoch() ).count()
      );
      CHECK( str == expected );
    }
  }
}

// https://questdb.io/docs/reference/api/ilp/overview/
SCENARIO( "ILP InfluxDB Test Suite" )
{
  GIVEN( "ILP Builder instance" )
  {
    auto builder = spt::ilp::Builder{};
    WHEN( "Testing all-fields-present-no-escapes" )
    {
      auto str = builder.
          startRecord( "somename"sv ).
          addTag( "tag1"sv, "val1"sv ).
          addTag( "tag2"sv, "val2"sv ).
          addValue( "floatfield"sv, double( 1 ) ).
          addValue( "strfield"sv, "hello"sv ).
          addValue( "intfield"sv, -1 ).
          addValue( "uintfield"sv, uint32_t( 1 ) ).
          addValue( "boolfield"sv, true ).
          timestamp( 1602841605822791506ns ).
          endRecord().
          finish();

      auto expected = "somename,tag1=val1,tag2=val2 floatfield=1,strfield=\"hello\",intfield=-1i,uintfield=1u,boolfield=true 1602841605822791506\n";
      REQUIRE( str == expected );
    }

    AND_WHEN( "Testing multiple-entries" )
    {
      auto str = builder.
          startRecord( "m1" ).
            addTag( "tag1"sv, "val1"sv ).
            addValue( "x"sv, "first"sv ).
            timestamp( 1602841605822791506ns ).
          endRecord().
          startRecord( "m2" ).
            addTag( "foo"sv, "bar"sv ).
            addValue( "x"sv, "second"sv ).
            timestamp( 1602841605822792000ns ).
          endRecord().
          finish();

      auto expected = R"(m1,tag1=val1 x="first" 1602841605822791506
m2,foo=bar x="second" 1602841605822792000
)";

      REQUIRE( str == expected );
    }

    AND_WHEN( "Testing escaped-values" )
    {
      auto str = builder.
          startRecord( "comma"sv ).
          addTag( "equals"sv, "e,x"sv ).
          addTag( "two"sv, "val2"sv ).
          addValue( "field"sv, std::string_view{ R"(fir"\n,st\)" } ).
          timestamp( 1602841605822791506ns ).
          endRecord().
          finish();

      auto expected = R"(comma,equals=e\,x,two=val2 field="fir\"\\n\,st\\" 1602841605822791506
)";

      REQUIRE( str == expected );
    }
  }
}
