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
  struct WriteConcern
  {
    enum class Level : uint8_t { Default, Majority, Tag, Unacknowledged, Acknowledged };

    explicit WriteConcern( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    WriteConcern() = default;
    ~WriteConcern() = default;
    WriteConcern(WriteConcern&&) = default;
    WriteConcern& operator=(WriteConcern&&) = default;
    bool operator==(const WriteConcern&) const = default;

    WriteConcern(const WriteConcern&) = delete;
    WriteConcern& operator=(const WriteConcern&) = delete;

    BEGIN_VISITABLES(WriteConcern);
    Level acknowledgeLevel{Level::Default};
    VISITABLE(std::string, tag);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, majority, {std::nullopt});
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::milliseconds>, timeout, {std::nullopt});
    VISITABLE(std::optional<int32_t>, nodes);
    VISITABLE_DIRECT_INIT(bool, journal, {false});
    END_VISITABLES;
  };

  void populate( WriteConcern& model, bsoncxx::document::view bson );
  void populate( const WriteConcern& model, bsoncxx::builder::stream::document& builder );
  void populate( const WriteConcern& model, boost::json::object& object );
}
