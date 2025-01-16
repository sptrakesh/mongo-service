//
// Created by Rakesh on 19/12/2024.
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

#include <bsoncxx/oid.hpp>

namespace spt::mongoservice::api::model::response
{
  struct Transaction
  {
    struct History
    {
      History() = default;
      ~History() = default;
      History(History&&) = default;
      History& operator=(History&&) = default;
      bool operator==(const History& rhs) const = default;

      History(const History&) = delete;
      History& operator=(const History&) = delete;

      BEGIN_VISITABLES(History);
      VISITABLE(std::vector<bsoncxx::oid>, created);
      VISITABLE(std::vector<bsoncxx::oid>, updated);
      VISITABLE(std::vector<bsoncxx::oid>, deleted);
      VISITABLE(std::string, database);
      VISITABLE(std::string, collection);
      END_VISITABLES;
    };

    explicit Transaction( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Transaction() = default;
    ~Transaction() = default;
    Transaction(Transaction&&) = default;
    Transaction& operator=(Transaction&&) = default;

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    BEGIN_VISITABLES(Transaction);
    VISITABLE(History, history);
    VISITABLE(int32_t, created);
    VISITABLE(int32_t, updated);
    VISITABLE(int32_t, deleted);
    END_VISITABLES;
  };
}
