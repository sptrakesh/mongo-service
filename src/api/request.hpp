//
// Created by Rakesh on 22/02/2022.
//

#pragma once

#include "model/request/action.hpp"

#if defined __has_include
  #if __has_include("../common/util/serialise.hpp")
    #include "../common/util/serialise.hpp"
  #else
    #include <mongo-service/common/util/serialise.hpp>
  #endif
#endif

#include <optional>
#include <string>
#include <string_view>
#include <bsoncxx/document/value.hpp>

namespace spt::mongoservice::api
{
  struct Request
  {
    Request( std::string_view db, std::string_view coll, bsoncxx::document::value doc, model::request::Action act ) :
        database{ db }, collection{ coll }, document{ std::move( doc ) }, action{ act } {}
    ~Request() = default;
    Request(Request&&) = default;
    Request& operator=(Request&&) = default;

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    static Request create( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::create };
    }

    static Request createTimeseries( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::createTimeseries };
    }

    static Request retrieve( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::retrieve };
    }

    static Request update( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::update };
    }

    static Request _delete( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::_delete };
    }

    static Request count( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::count };
    }

    static Request distinct( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::distinct };
    }

    static Request index( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::index };
    }

    static Request dropCollection( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::dropCollection };
    }

    static Request dropIndex( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::dropIndex };
    }

    static Request bulk( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::bulk };
    }

    static Request pipeline( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::pipeline };
    }

    static Request transaction( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::transaction };
    }

    static Request createCollection( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::createCollection };
    }

    static Request renameCollection( std::string_view db, std::string_view coll, bsoncxx::document::value doc )
    {
      return { db, coll, std::move( doc ), model::request::Action::renameCollection };
    }

    std::string database;
    std::string collection;
    bsoncxx::document::value document;
    std::optional<bsoncxx::document::value> options{ std::nullopt };
    std::optional<bsoncxx::document::value> metadata{ std::nullopt };
    std::optional<std::string> correlationId{ std::nullopt };
    model::request::Action action{ model::request::Action::retrieve };
    bool skipVersion{ false };
    bool skipMetric{ false };
  };
}