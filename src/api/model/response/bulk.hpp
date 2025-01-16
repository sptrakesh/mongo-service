//
// Created by Rakesh on 18/12/2024.
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

#include <cstdint>

namespace spt::mongoservice::api::model::response
{
  struct Bulk
  {
    explicit Bulk( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Bulk() = default;
    ~Bulk() = default;
    Bulk(Bulk&&) = default;
    Bulk& operator=(Bulk&&) = default;

    Bulk(const Bulk&) = delete;
    Bulk& operator=(const Bulk&) = delete;

    BEGIN_VISITABLES(Bulk);
    VISITABLE(int32_t, create);
    VISITABLE(int32_t, remove);
    VISITABLE(int32_t, history);
    END_VISITABLES;
  };
}
