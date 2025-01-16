//
// Created by Rakesh on 17/12/2024.
//

#pragma once

#include "history.hpp"

namespace spt::mongoservice::api::model::response
{
  template <util::Visitable Document>
  requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
  struct Update
  {
    explicit Update( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    Update() = default;
    ~Update() = default;
    Update(Update&&) = default;
    Update& operator=(Update&&) = default;

    Update(const Update&) = delete;
    Update& operator=(const Update&) = delete;

    void populate( bsoncxx::document::view bson )
    {
      FROM_BSON( bool, skipVersion, bson );
    }

    BEGIN_VISITABLES(Update);
    VISITABLE(std::optional<Document>, document);
    VISITABLE(std::optional<History>, history);
    std::optional<bool> skipVersion;
    END_VISITABLES;
  };

  struct UpdateMany
  {
    explicit UpdateMany( bsoncxx::document::view document ) { util::unmarshall( *this, document ); }
    UpdateMany() = default;
    ~UpdateMany() = default;
    UpdateMany(UpdateMany&&) = default;
    UpdateMany& operator=(UpdateMany&&) = default;

    UpdateMany(const UpdateMany&) = delete;
    UpdateMany& operator=(const UpdateMany&) = delete;

    BEGIN_VISITABLES(UpdateMany);
    VISITABLE(std::vector<bsoncxx::oid>, success);
    VISITABLE(std::vector<bsoncxx::oid>, failure);
    VISITABLE(std::vector<History>, history);
    END_VISITABLES;
  };

  template <util::Visitable Document>
  void populate( Update<Document>& model, bsoncxx::document::view bson )
  {
    model.populate( bson );
  }
}
