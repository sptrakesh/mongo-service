//
// Created by Rakesh on 17/12/2024.
//

#pragma once

#include "../../options/dropcollection.hpp"
#include "action.hpp"

namespace spt::mongoservice::api::model::request
{
  struct RenameCollection
  {
    struct Document
    {
      explicit Document( std::string target ) : target{ std::move( target ) } {}
      Document() = default;
      ~Document() = default;
      Document(Document&&) = default;
      Document& operator=(Document&&) = default;
      bool operator==(const Document&) const = default;

      Document(const Document&) = delete;
      Document& operator=(const Document&) = delete;

      BEGIN_VISITABLES(Document);
      VISITABLE(std::string, target);
      END_VISITABLES;
    };

    explicit RenameCollection( std::string target ) : document{ std::move( target ) } {}
    RenameCollection() = default;
    ~RenameCollection() = default;
    RenameCollection(RenameCollection&&) = default;
    RenameCollection& operator=(RenameCollection&&) = default;

    RenameCollection(const RenameCollection&) = delete;
    RenameCollection& operator=(const RenameCollection&) = delete;

    BEGIN_VISITABLES(RenameCollection);
    VISITABLE(Document, document);
    VISITABLE(std::optional<options::DropCollection>, options);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(std::string, application);
    VISITABLE(std::string, correlationId);
    VISITABLE_DIRECT_INIT(Action, action, {Action::renameCollection});
    VISITABLE_DIRECT_INIT(bool, skipMetric, {false});
    END_VISITABLES;
  };
}
