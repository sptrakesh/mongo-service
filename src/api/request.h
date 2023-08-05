//
// Created by Rakesh on 22/02/2022.
//

#pragma once

#include <optional>
#include <string>
#include <bsoncxx/document/value.hpp>

namespace spt::mongoservice::api
{
  struct Request
  {
    enum class Action : std::uint_fast8_t {
      create, retrieve, update, _delete, count,
      index, dropCollection, dropIndex,
      bulk, pipeline, transaction, renameCollection
    };

    Request( std::string db, std::string coll,
        bsoncxx::document::value doc, Action act ) :
        database{ std::move( db ) }, collection{ std::move( coll ) },
        document{ std::move( doc ) }, action{ act } {}
    ~Request() = default;
    Request(Request&&) = default;
    Request& operator=(Request&&) = default;
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    static Request create( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::create };
    }

    static Request retrieve( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::retrieve };
    }

    static Request update( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::update };
    }

    static Request _delete( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::_delete };
    }

    static Request count( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::count };
    }

    static Request index( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::index };
    }

    static Request dropCollection( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::dropCollection };
    }

    static Request dropIndex( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::dropIndex };
    }

    static Request bulk( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::bulk };
    }

    static Request pipeline( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::pipeline };
    }

    static Request transaction( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::transaction };
    }

    static Request renameCollection( std::string db, std::string coll, bsoncxx::document::value doc )
    {
      return { std::move( db ), std::move( coll ), std::move( doc ), Action::renameCollection };
    }

    std::string database;
    std::string collection;
    bsoncxx::document::value document;
    std::optional<bsoncxx::document::value> options{ std::nullopt };
    std::optional<bsoncxx::document::value> metadata{ std::nullopt };
    std::optional<std::string> correlationId{ std::nullopt };
    Action action{ Action::retrieve };
    bool skipVersion{ false };
    bool skipMetric{ false };
  };
}