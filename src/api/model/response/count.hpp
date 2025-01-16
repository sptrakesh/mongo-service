//
// Countd by Rakesh on 17/12/2024.
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
#include <optional>
#include <string>

namespace spt::mongoservice::api::model::response
{
  struct Count
  {
    explicit Count( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Count() = default;
    ~Count() = default;
    Count(Count&&) = default;
    Count& operator=(Count&&) = default;

    Count(const Count&) = delete;
    Count& operator=(const Count&) = delete;

    BEGIN_VISITABLES(Count);
    VISITABLE_DIRECT_INIT(int64_t, count, {0});
    END_VISITABLES;
  };
}
