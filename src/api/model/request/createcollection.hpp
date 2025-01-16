//
// Created by Rakesh on 17/12/2024.
//

#pragma once

#include "../../options/createcollection.hpp"
#include "action.hpp"

namespace spt::mongoservice::api::model::request
{
  struct CreateCollection
  {
    explicit CreateCollection( options::CreateCollection document ) : document{ std::move( document ) } {}
    CreateCollection() = default;
    ~CreateCollection() = default;
    CreateCollection(CreateCollection&&) = default;
    CreateCollection& operator=(CreateCollection&&) = default;

    CreateCollection(const CreateCollection&) = delete;
    CreateCollection& operator=(const CreateCollection&) = delete;

    BEGIN_VISITABLES(CreateCollection);
    VISITABLE(std::optional<options::CreateCollection>, document);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(std::string, application);
    VISITABLE(std::string, correlationId);
    VISITABLE_DIRECT_INIT(Action, action, {Action::createCollection});
    VISITABLE_DIRECT_INIT(bool, skipMetric, {false});
    END_VISITABLES;
  };
}
