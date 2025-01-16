//
// Created by Rakesh on 23/12/2024.
//

#include "repository.hpp"

auto spt::mongoservice::api::repository::transaction( bsoncxx::document::value request ) -> std::expected<model::response::Transaction, Error>
{
  using O = std::expected<model::response::Transaction, Error>;
  using std::operator ""sv;

  try
  {
    if ( !util::bsonValueIfExists<bsoncxx::document::view>( "document", request.view() ) )
    {
      LOG_WARN << "No document in payload";
      return O{ std::unexpect, "No document in payload"sv, Error::Cause::data };
    }

    if ( auto doc = util::bsonValue<bsoncxx::document::view>( "document", request.view() );
      !util::bsonValueIfExists<bsoncxx::array::view>( "items", doc ) )
    {
      LOG_WARN << "No items array in payload";
      return O{ std::unexpect, "No items array in payload"sv, Error::Cause::data };
    }

    const auto [type, opt] = execute( request.view() );
    if ( type == ResultType::poolFailure )
    {
      LOG_WARN << "Connection pool exhausted while executing transaction " << boost::json::serialize( util::toJson( request ) );
      return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
    }

    if ( type == ResultType::commandFailure )
    {
      LOG_WARN << "Command returned no data while executing transaction " << util::json::str( request );
      return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
    }

    if ( !opt )
    {
      LOG_WARN << "API returned no data while executing transaction " << util::json::str( request );
      return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
    }

    if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
    {
      LOG_WARN << "API returned error while executing transaction " << *err <<
        ". " << util::json::str( request ) <<
        ". " << boost::json::serialize( util::toJson( opt->view() ) );
      return O{ std::unexpect, *err, Error::Cause::data };
    }

    return O{ std::in_place, opt->view() };
  }
  catch( const std::exception& ex )
  {
    LOG_WARN << "Error executing transaction " << ex.what() << ". " << util::json::str( request );
    return O{ std::unexpect, ex.what(), Error::Cause::exception };
  }
  catch ( ... )
  {
    LOG_WARN << "Unknown error executing transaction " << util::json::str( request );
    return O{ std::unexpect, "Unknown error executing transaction."sv, Error::Cause::exception };
  }
}

auto spt::mongoservice::api::repository::dropCollection( model::request::DropCollection& request ) -> std::expected<model::response::DropCollection, Error>
{
  using O = std::expected<model::response::DropCollection, Error>;
  using std::operator ""sv;

  try
  {
    request.application = impl::ApiSettings::instance().application;
    auto bson = util::marshall( request );
    const auto [type, opt] = execute( bson );
    if ( type == ResultType::poolFailure )
    {
      LOG_WARN << "Connection pool exhausted dropping collection " << util::json::str( request );
      return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
    }

    if ( type == ResultType::commandFailure )
    {
      LOG_WARN << "Command returned no data while dropping collection " << util::json::str( request );
      return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
    }

    if ( !opt )
    {
      LOG_WARN << "API returned no data while dropping collection " << util::json::str( request );
      return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
    }

    if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
    {
      LOG_WARN << "API returned error while dropping collection " << *err <<
        ". " << util::json::str( request ) <<
        ". " << boost::json::serialize( util::toJson( opt->view() ) );
      return O{ std::unexpect, *err, Error::Cause::data };
    }

    return O{ std::in_place, opt->view() };
  }
  catch( const std::exception& ex )
  {
    LOG_WARN << "Error dropping collection " << ex.what() << ". " << util::json::str( request );
    return O{ std::unexpect, ex.what(), Error::Cause::exception };
  }
  catch ( ... )
  {
    LOG_WARN << "Unknown error dropping collection for " << util::json::str( request );
    return O{ std::unexpect, "Unknown error dropping collection."sv, Error::Cause::exception };
  }
}
