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

#include <optional>
#include <bsoncxx/oid.hpp>

namespace spt::mongoservice::api::model::response
{
  struct Create
  {
    explicit Create( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Create() = default;
    ~Create() = default;
    Create(Create&&) = default;
    Create& operator=(Create&&) = default;

    Create(const Create&) = delete;
    Create& operator=(const Create&) = delete;

    BEGIN_VISITABLES(Create);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(std::optional<bsoncxx::oid>, id);
    VISITABLE(std::optional<bsoncxx::oid>, entity);
    VISITABLE(std::optional<bool>, skipVersion);
    END_VISITABLES;
  };
}
