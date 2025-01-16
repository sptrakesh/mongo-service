//
// Retrieved by Rakesh on 17/12/2024.
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

namespace spt::mongoservice::api::model::response
{
  template <util::Visitable Document>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
  struct Retrieve
  {
    explicit Retrieve( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Retrieve() = default;
    ~Retrieve() = default;
    Retrieve(Retrieve&&) = default;
    Retrieve& operator=(Retrieve&&) = default;

    Retrieve(const Retrieve&) = delete;
    Retrieve& operator=(const Retrieve&) = delete;

    BEGIN_VISITABLES(Retrieve);
    VISITABLE(std::optional<Document>, result);
    VISITABLE(std::vector<Document>, results);
    END_VISITABLES;
  };
}
