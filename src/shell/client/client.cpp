//
// Created by Rakesh on 29/12/2021.
//

#include "client.hpp"
#include "../../api/api.hpp"
#include "../../common/util/bson.hpp"
#include "../../log/NanoLog.hpp"

#include <cctype>
#include <cstring>
#include <iostream>
#include <string_view>

#include <readline/readline.h>
#include <readline/history.h>

//https://stackoverflow.com/questions/2924697/how-does-one-output-bold-text-in-bash

namespace
{
  namespace pclient
  {
    void help()
    {
      std::cout << "\033[1mInteract with mongo-service\033[0m" << '\n';
      std::cout << "  \033[1mTo create document\033[0m {\"action\":\"create\",\"database\":\"<database name>\",\"collection\":\"<collection name>\",\"document\":{\"_id\":{\"$oid\":\"5f35e5e1e799c52186039122\"},\"intValue\":123,\"floatValue\":123.0,\"boolValue\":true,\"stringValue\":\"abc123\",\"nested\":{\"key\":\"value\"}}}\n";
      std::cout << "  \033[1mTo retrieve\033[0m {\"action\":\"retrieve\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"_id\":{\"$oid\":\"5f35e6d8c7e3a976365b3751\"}}}\n";
      std::cout << "  \033[1mTo count\033[0m {\"action\":\"count\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{}}\n";
      std::cout << "  \033[1mDistinct values\033[0m {\"action\":\"distinct\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"field\":\"myProp\",\"filter\":{\"deleted\":{\"$ne\":true}}}}\n";
      std::cout << "  \033[1mUpdate by id\033[0m {\"action\":\"update\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"key1\":\"value1\",\"_id\":{\"$oid\":\"5f35e887bb516401e02b4701\"}}}\n";
      std::cout << "  \033[1mUpdate without history\033[0m {\"action\":\"update\",\"database\":\"itest\",\"collection\":\"test\",\"skipVersion\":true,\"document\":{\"key1\":\"value1\",\"_id\":{\"$oid\":\"5f35e932d3698352cb3bd2d1\"}}}\n";
      std::cout << "  \033[1mReplace document\033[0m {\"action\":\"update\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"filter\":{\"_id\":{\"$oid\":\"5f3bc9e2502422053e08f9f1\"}},\"replace\":{\"_id\":{\"$oid\":\"5f3bc9e2502422053e08f9f1\"},\"key\":\"value\",\"key1\":\"value1\"}},\"options\":{\"upsert\":true},\"application\":\"version-history-api\",\"metadata\":{\"revertedFrom\":{\"$oid\":\"5f3bc9e29ba4f45f810edf29\"}}}\n";
      std::cout << "  \033[1mUpdate with unset\033[0m {\"action\":\"update\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"filter\":{\"_id\":{\"$oid\":\"6435a62316d2310e800e4bf2\"}},\"update\":{\"$unset\":{\"obsoleteProperty\":1},\"$set\":{\"metadata.modified\":{\"$date\":1681237539583},\"metadata.user._id\":{\"$oid\":\"5f70ee572fc09200086c8f24\"},\"metadata.user.username\":\"mqtt\"}}}}\n";
      std::cout << "  \033[1mDelete document\033[0m {\"action\":\"delete\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"_id\":{\"$oid\":\"5f35ea61aa4ef01184492d71\"}}}\n";
      std::cout << "  \033[1mDrop collection\033[0m {\"action\":\"dropCollection\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"clearVersionHistory\":true}}\n";
      std::cout << "  \033[1mBulk create\033[0m {\"action\":\"bulk\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"insert\":[{\"_id\":{\"$oid\":\"5f6ba5f9de326c57bd64efb1\"},\"key\":\"value1\"},{\"_id\":{\"$oid\":\"5f6ba5f9de326c57bd64efb2\"},\"key\":\"value2\"}],\"remove\":[{\"_id\":{\"$oid\":\"5f6ba5f9de326c57bd64efb1\"}}]}}\n";
      std::cout << "  \033[1mAggregation pipeline\033[0m {\"action\":\"pipeline\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"specification\":[{\"$match\":{\"_id\":{\"$oid\":\"5f861c8452c8ca000d60b783\"}}},{\"$sort\":{\"_id\":-1}},{\"$limit\":20},{\"$lookup\":{\"localField\":\"user._id\",\"from\":\"user\",\"foreignField\":\"_id\",\"as\":\"users\"}},{\"$lookup\":{\"localField\":\"group._id\",\"from\":\"group\",\"foreignField\":\"_id\",\"as\":\"groups\"}}]}}\n";
      std::cout << "  \033[1mCreate timeseries\033[0m {\"action\":\"createTimeseries\",\"database\":\"<database name>\",\"collection\":\"<collection name>\",\"document\":{\"value\":123.456,\"tags\":{\"property1\":\"string\",\"property2\":false},\"timestamp\":\"2024-11-21T17:36:28Z\"}}\n";
      std::cout << "  \033[1mCreate collection\033[0m {\"action\":\"createCollection\",\"database\":\"itest\",\"collection\":\"timeseries\",\"document\":{\"timeseries\":{\"timeField\":\"date\",\"metaField\":\"tags\",\"granularity\":\"minutes\"}}}\n";
      std::cout << "  \033[1mRename collection\033[0m {\"action\":\"renameCollection\",\"database\":\"itest\",\"collection\":\"test\",\"document\":{\"target\":\"test-renamed\"}}\n";
    }

    std::string_view trim( std::string_view in )
    {
      auto left = in.begin();
      for ( ;; ++left )
      {
        if ( left == in.end() ) return in;
        if ( !std::isspace( *left ) ) break;
      }
      auto right = in.end() - 1;
      for ( ; right > left && std::isspace( *right ); --right );
  #if defined(_WIN32) || defined(WIN32)
      return { left, right };
  #else
      return { left, static_cast<std::size_t>(std::distance( left, right ) + 1) };
  #endif
    }

    void run( std::string_view payload )
    {
      using spt::util::bsonValueIfExists;
      using spt::mongoservice::api::execute;
      using spt::mongoservice::api::Request;

      try
      {
        LOG_INFO << "Executing " << payload;
        auto bson = bsoncxx::from_json( payload );

        auto act = bsonValueIfExists<std::string>( "action", bson.view() );
        if ( !act )
        {
          std::cout << "\033[1mAction not specified\033[0m\n";
          return;
        }

        auto action = magic_enum::enum_cast<spt::mongoservice::api::model::request::Action>( *act );
        if ( !action )
        {
          std::cout << "\033[1mInvalid action\033[0m " << *act << "\n";
          return;
        }

        auto db = bsonValueIfExists<std::string>( "database", bson.view() );
        if ( !db )
        {
          std::cout << "\033[1mDatabase not specified\033[0m\n";
          return;
        }

        auto coll = bsonValueIfExists<std::string>( "collection", bson.view() );
        if ( !coll )
        {
          std::cout << "\033[1mCollection not specified\033[0m\n";
          return;
        }

        auto doc = bsonValueIfExists<bsoncxx::document::view>( "document", bson.view() );
        if ( !act )
        {
          std::cout << "\033[1mDocument not specified\033[0m\n";
          return;
        }

        auto req = Request( *db, *coll, bsoncxx::document::value{ *doc }, *action );

        if ( auto options = bsonValueIfExists<bsoncxx::document::view>( "options", bson.view() ); options )
        {
          req.options.emplace( *options );
        }

        if ( auto metadata = bsonValueIfExists<bsoncxx::document::view>( "metadata", bson.view() ); metadata )
        {
          req.metadata.emplace( *metadata );
        }

        if ( auto corr = bsonValueIfExists<std::string>( "correlationId", bson.view() ); corr )
        {
          req.correlationId = *corr;
        }

        if ( auto skip = bsonValueIfExists<bool>( "skipVersion", bson.view() ); skip )
        {
          req.skipVersion = *skip;
        }

        if ( auto skip = bsonValueIfExists<bool>( "skipMetric", bson.view() ); skip )
        {
          req.skipMetric = *skip;
        }

        const auto [type, opt] = execute( req );
        if ( !opt )
        {
          LOG_WARN << "Unable to execute payload against " << *db << ':' << *coll << ". " << payload;
          std::cout << "Error executing payload";
          return;
        }

        const auto view = opt->view();
        if ( const auto err = spt::util::bsonValueIfExists<std::string>( "error", view ); err )
        {
          LOG_WARN << "Error executing query against " << *db << ':' << *coll << ". " << *err << ". " << payload;
          std::cout << "Error executing payload";
          return;
        }

        if ( const auto res = bsonValueIfExists<bsoncxx::document::view>( "result", view ); res )
        {
          std::cout << bsoncxx::to_json( *res ) << '\n';
          return;
        }

        if ( const auto res = bsonValueIfExists<bsoncxx::array::view>( "results", view ); res )
        {
          std::cout << bsoncxx::to_json( *res ) << '\n';
        }
      }
      catch ( const std::exception& e )
      {
        std::cerr << e.what() << '\n';
      }
    }
  }
}

int spt::mongoservice::client::run( std::string_view server, std::string_view port )
{
  api::init( server, port, "mongo-service-shell" );

  using namespace std::literals;
  std::cout << "Enter commands followed by <ENTER>\n";
  std::cout << "Enter \033[1mhelp\033[0m for help about commands\n";
  std::cout << "\033[1mEnter minified JSON payload followed by new line!\033[0m\n";
  std::cout << "Enter \033[1mexit\033[0m or \033[1mquit\033[0m to exit shell\n";

  // Disable tab completion
  rl_bind_key( '\t', rl_insert );

  std::string previous;
  previous.reserve( 128 );

  char* buf;
  while ( ( buf = readline("mongo-service> " ) ) != nullptr )
  {
    auto view = std::string_view( buf );
    if ( view.empty() )
    {
      std::free( buf );
      continue;
    }

    if ( std::string_view{ previous } != view ) add_history( buf );
    view = pclient::trim( view );

    if ( view == "exit"sv || view == "quit"sv )
    {
      std::cout << "Bye\n";
      break;
    }
    else if ( view == "help"sv ) pclient::help();
    else if ( view.empty() ) { /* noop */ }
    else pclient::run( view );

    previous.clear();
    previous.append( buf, view.size() );
    std::free( buf );
  }

  return 0;
}
