//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#if defined __has_include
  #if __has_include("../common/visit_struct/visit_struct_intrusive.hpp")
    #include "../common/visit_struct/visit_struct_intrusive.hpp"
    #include "../common/util/serialise.hpp"
  #else
    #include <mongo-service/common/visit_struct/visit_struct_intrusive.hpp>
    #include <mongo-service/common/util/serialise.hpp>
  #endif
#endif

namespace spt::mongoservice::api::options
{
  struct Collation
  {
    explicit Collation( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }

    Collation() = default;
    ~Collation() = default;
    Collation(Collation&&) = default;
    Collation& operator=(Collation&&) = default;
    bool operator==(const Collation&) const = default;

    Collation(const Collation&) = delete;
    Collation& operator=(const Collation&) = delete;

    BEGIN_VISITABLES(Collation);
    VISITABLE(std::string, locale);
    VISITABLE(std::string, caseFirst);
    VISITABLE(std::string, alternate);
    VISITABLE(std::string, maxVariable);
    VISITABLE(std::optional<int32_t>, strength);
    VISITABLE(std::optional<bool>, caseLevel);
    VISITABLE(std::optional<bool>, numericOrdering);
    VISITABLE(std::optional<bool>, backwards);
    END_VISITABLES;
  };
}