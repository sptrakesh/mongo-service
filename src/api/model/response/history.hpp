//
// Created by Rakesh on 17/12/2024.
//

#pragma once

#if defined __has_include
  #if __has_include("../../../common/visit_struct/visit_struct_intrusive.hpp")
    #include "../../../common/visit_struct/visit_struct_intrusive.hpp"
    #include "../../../common/util/serialise.hpp"
  #else
    #include <mongo-service/common/visit_struct/visit_struct_intrusive.hpp>
    #include <mongo-service/common/util/serialise.hpp>
  #endif
#endif

#include <string>
#include <bsoncxx/oid.hpp>

namespace spt::mongoservice::api::model::response
{
  struct History
  {
    History() = default;
    ~History() = default;
    History(History&&) = default;
    History& operator=(History&&) = default;
    bool operator==(const History& other) const = default;

    History(const History&) = delete;
    History& operator=(const History&) = delete;

    BEGIN_VISITABLES(History);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(bsoncxx::oid, id);
    VISITABLE(bsoncxx::oid, entity);
    END_VISITABLES;
  };
}