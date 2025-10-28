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
  struct ReadPreference
  {
    enum class ReadMode : uint8_t { Primary, PrimaryPreferred, Secondary, SecondaryPreferred, Nearest };

    explicit ReadPreference( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    ReadPreference() = default;
    ~ReadPreference() = default;
    ReadPreference(ReadPreference&&) = default;
    ReadPreference& operator=(ReadPreference&&) = default;
    bool operator==(const ReadPreference&) const = default;

    ReadPreference(const ReadPreference&) = delete;
    ReadPreference& operator=(const ReadPreference&) = delete;

    BEGIN_VISITABLES(ReadPreference);
    VISITABLE(std::optional<bsoncxx::document::value>, tags);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, maxStaleness, {std::nullopt});
    ReadMode mode{ReadMode::Primary};
    END_VISITABLES;
  };

  void populate( ReadPreference& model, bsoncxx::document::view bson );
  void populate( const ReadPreference& model, bsoncxx::builder::stream::document& builder );
  void populate( const ReadPreference& model, boost::json::object& object );

  struct ReadConcern
  {
    enum class Level : uint_fast8_t { local, available, majority, linearizable, snapshot };

    explicit ReadConcern( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    ReadConcern() = default;
    ~ReadConcern() = default;
    ReadConcern(ReadConcern&&) = default;
    ReadConcern& operator=(ReadConcern&&) = default;
    bool operator==(const ReadConcern&) const = default;

    ReadConcern(const ReadConcern&) = delete;
    ReadConcern& operator=(const ReadConcern&) = delete;

    BEGIN_VISITABLES(ReadConcern);
    VISITABLE_DIRECT_INIT(Level, level, {Level::local});
    END_VISITABLES;
  };
}
