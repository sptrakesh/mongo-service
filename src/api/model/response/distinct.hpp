//
// Distinctd by Rakesh on 17/12/2024.
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
#include <string>
#include <vector>

namespace spt::mongoservice::api::model::response
{
  struct Distinct
  {
    struct Value
    {
      Value() = default;
      ~Value() = default;
      Value(Value&&) = default;
      Value& operator=(Value&&) = default;
      bool operator==(const Value& other) const = default;

      Value(const Value&) = delete;
      Value& operator=(const Value&) = delete;

      BEGIN_VISITABLES(Value);
      VISITABLE(std::vector<std::string>, values);
      VISITABLE(int64_t, ok);
      END_VISITABLES;
    };

    explicit Distinct( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Distinct() = default;
    ~Distinct() = default;
    Distinct(Distinct&&) = default;
    Distinct& operator=(Distinct&&) = default;

    Distinct(const Distinct&) = delete;
    Distinct& operator=(const Distinct&) = delete;

    BEGIN_VISITABLES(Distinct);
    VISITABLE(std::vector<Value>, results);
    END_VISITABLES;
  };
}
