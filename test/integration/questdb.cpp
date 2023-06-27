//
// Created by Rakesh on 03/01/2023.
//

#include "../../src/api/contextholder.h"
#include "../../src/ilp/builder.h"
#include "../../src/ilp/ilp.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <thread>
#include <cpr/cpr.h>

using std::operator""sv;

namespace spt::itest::ilp
{
  struct Docker
  {
    static const Docker& instance()
    {
      static Docker docker;
      return docker;
    }

    ~Docker()
    {
      LOG_INFO << "Stopping QuestDB docker container";
      std::system( "docker stop questdb" );
    }

  private:
    Docker()
    {
      LOG_INFO << "Starting QuestDB docker container";
      std::system( "docker run -d --rm -p 9000:9000 -p 8812:8812 -p 9009:9009 --name questdb questdb/questdb" );
      std::this_thread::sleep_for( std::chrono::seconds{ 2 } );
    }
  };

  struct Client
  {
    Client() : client{ spt::mongoservice::api::ContextHolder::instance().ioc, "127.0.0.1"sv, "9009"sv } {}

    ~Client()
    {
      spt::mongoservice::api::ContextHolder::instance().ioc.stop();
      if ( thread.joinable() ) thread.join();
    }

    void write( std::string&& data )
    {
      client.write( std::move( data ) );
    }

  private:
    std::thread thread{ [](){ spt::mongoservice::api::ContextHolder::instance().ioc.run(); } };
    spt::ilp::ILPClient client;
  };
}

SCENARIO( "QuestDB TCP client test suite" )
{
  [[maybe_unused]] auto& docker = spt::itest::ilp::Docker::instance();

  GIVEN( "A TCP client connected to QuestDB TCP service" )
  {
    auto client = spt::itest::ilp::Client{};

    WHEN( "Adding a test series to QuestDB" )
    {
      for ( int64_t i = 0; i < 100; ++i )
      {
        auto builder = spt::ilp::Builder{};

        for ( int64_t j = 0; j < 100; ++j )
        {
          builder.
            startRecord( "test"sv ).
              addTag( "i"sv, std::to_string( i ) ).
              addTag( "j"sv, std::to_string( j ) ).
              addValue( "value"sv, int64_t( i * j ) ).
            endRecord();
        }

        client.write( builder.finish() );
        LOG_INFO << "Added batch of 100 records " << i + 1 << "/100";
      }
    }

    AND_WHEN( "Retrieving test series from QuestDB" )
    {
      LOG_INFO << "Sleep 5 seconds for data to be flushed";
      std::this_thread::sleep_for( std::chrono::seconds{ 5 } );
      auto resp = cpr::Get(cpr::Url{"http://localhost:9000/exec"},
          cpr::Parameters{{"count", "true"}, {"query", "select * from test;"}});
      REQUIRE( resp.text.find( "\"count\":10000" ) != std::string::npos );
    }
  }
}
