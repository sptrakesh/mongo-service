//
// Created by Rakesh on 05/09/2024.
//

#include "../../src/log/NanoLog.hpp"
#include "../../src/log/stacktrace.hpp"

#include <ranges>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <bsoncxx/oid.hpp>
#include <catch2/catch_test_macros.hpp>

namespace spt::log::pstacktrace
{
  void generate()
  {
    throw std::runtime_error( spt::log::stacktrace() );
  }

  void delegate()
  {
    generate();
  }

  void nested()
  {
    delegate();
  }
}

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

SCENARIO( "Stacktrace test suite", "[stacktrace]" )
{
  GIVEN( "Utility function to return stacktrace" )
  {
    WHEN( "Generating current stacktrace" )
    {
      const auto st = spt::log::stacktrace();
      LOG_INFO << st;
      INFO( st );
      CHECK_FALSE( st.empty() );

      auto parts = std::vector<std::string>{};
      parts.reserve( 8 );
      boost::split( parts, st, boost::is_any_of( "\r\n" ) );
      CHECK( parts.size() > 3 );
    }

    AND_WHEN( "Generating stactrace from exception" )
    {
      try
      {
        spt::log::pstacktrace::nested();
        FAIL( "Exception not thrown" );
      }
      catch ( const std::exception& e )
      {
        LOG_INFO << e.what();

        auto parts = std::vector<std::string>{};
        parts.reserve( 8 );
        boost::split( parts, e.what(), boost::is_any_of( "\r\n" ) );
        CHECK( parts.size() > 4 );

        auto iter = std::ranges::find_if( parts, []( const auto& s ) { return s.contains( "spt::log::pstacktrace::generate" ); });
        auto status = iter != std::ranges::end( parts );
        CHECK( status );

        iter = std::ranges::find_if( parts, []( const auto& s ) { return s.contains( "spt::log::pstacktrace::delegate" ); });
        status = iter != std::ranges::end( parts );
        CHECK( status );

        iter = std::ranges::find_if( parts, []( const auto& s ) { return s.contains( "spt::log::pstacktrace::nested" ); });
        status = iter != std::ranges::end( parts );
        CHECK( status );
      }
    }
  }
}