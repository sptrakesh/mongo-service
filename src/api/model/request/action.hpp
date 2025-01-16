//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include <cstdint>
#include <bsoncxx/types/bson_value/value.hpp>

namespace spt::mongoservice::api::model::request
{
  enum class Action : std::uint_fast8_t {
    create, createTimeseries, retrieve, update, _delete, count, distinct,
    createCollection, renameCollection, dropCollection, index, dropIndex,
    bulk, pipeline, transaction,
    invalid = 255
  };
}

namespace spt::util
{
  bsoncxx::types::bson_value::value bson( const mongoservice::api::model::request::Action& action );
  void set( mongoservice::api::model::request::Action& field, bsoncxx::types::bson_value::view value );
}