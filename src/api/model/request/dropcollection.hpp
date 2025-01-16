//
// Created by Rakesh on 13/12/2024.
//

#pragma once

#include "../../options/dropcollection.hpp"
#include "action.hpp"

namespace spt::mongoservice::api::model::request
{
  struct DropCollection
  {
    struct Document
    {
      bool operator==(const Document&) const = default;
      BEGIN_VISITABLES(Document);
      VISITABLE_DIRECT_INIT(bool, clearVersionHistory, {false});
      END_VISITABLES;
    };

    DropCollection() = default;
    ~DropCollection() = default;
    DropCollection(DropCollection&&) = default;
    DropCollection& operator=(DropCollection&&) = default;

    DropCollection(const DropCollection&) = delete;
    DropCollection& operator=(const DropCollection&) = delete;

    BEGIN_VISITABLES(DropCollection);
    VISITABLE(Document, document);
    VISITABLE(std::optional<options::DropCollection>, options);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(std::string, application);
    VISITABLE(std::string, correlationId);
    VISITABLE_DIRECT_INIT(Action, action, {Action::dropCollection});
    VISITABLE_DIRECT_INIT(bool, skipMetric, {false});
    END_VISITABLES;
  };
}
