//
// Created by Rakesh on 20/12/2024.
//

#pragma once

#include "collation.hpp"
#include "writeconcern.hpp"

namespace spt::mongoservice::api::options
{
  struct CreateCollection
  {
    enum class ValidationLevel : uint8_t { off, strict, moderate, invalid = 255 };
    enum class ValidationAction : uint8_t { warn, error, invalid = 255 };

    struct Timeseries
    {
      enum class Granularity : uint8_t { seconds, minutes, hours, invalid = 255 };

      explicit Timeseries( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
      Timeseries() = default;
      ~Timeseries() = default;
      Timeseries(Timeseries&&) = default;
      Timeseries& operator=(Timeseries&&) = default;
      bool operator==(const Timeseries&) const = default;

      Timeseries(const Timeseries&) = delete;
      Timeseries& operator=(const Timeseries&) = delete;

      BEGIN_VISITABLES(Timeseries);
      VISITABLE(std::string, timeField);
      VISITABLE(std::string, metaField);
      VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, bucketMaxSpanSeconds, {std::nullopt});
      VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, bucketRoundingSeconds, {std::nullopt});
      VISITABLE_DIRECT_INIT(Granularity, granularity, {Granularity::invalid});
      END_VISITABLES;
    };

    struct ChangeStream
    {
      explicit ChangeStream( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
      ChangeStream() = default;
      ~ChangeStream() = default;
      ChangeStream(ChangeStream&&) = default;
      ChangeStream& operator=(ChangeStream&&) = default;
      bool operator==(const ChangeStream&) const = default;

      ChangeStream(const ChangeStream&) = delete;
      ChangeStream& operator=(const ChangeStream&) = delete;

      BEGIN_VISITABLES(ChangeStream);
      VISITABLE_DIRECT_INIT(bool, enabled, {true});
      END_VISITABLES;
    };

    struct ClusteredIndex
    {
      struct Key
      {
        explicit Key( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
        Key() = default;
        ~Key() = default;
        Key(Key&&) = default;
        Key& operator=(Key&&) = default;
        bool operator==(const Key&) const = default;

        Key(const Key&) = delete;
        Key& operator=(const Key&) = delete;

        BEGIN_VISITABLES(Key);
        VISITABLE_DIRECT_INIT(int32_t, _id, {1});
        END_VISITABLES;
      };

      explicit ClusteredIndex( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
      ClusteredIndex() = default;
      ~ClusteredIndex() = default;
      ClusteredIndex(ClusteredIndex&&) = default;
      ClusteredIndex& operator=(ClusteredIndex&&) = default;
      bool operator==(const ClusteredIndex&) const = default;

      ClusteredIndex(const ClusteredIndex&) = delete;
      ClusteredIndex& operator=(const ClusteredIndex&) = delete;

      BEGIN_VISITABLES(ClusteredIndex);
      VISITABLE(Key, key);
      VISITABLE(std::string, name);
      VISITABLE_DIRECT_INIT(bool, unique, {true});
      END_VISITABLES;
    };

    explicit CreateCollection( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }
    CreateCollection() = default;
    ~CreateCollection() = default;
    CreateCollection(CreateCollection&&) = default;
    CreateCollection& operator=(CreateCollection&&) = default;

    CreateCollection(const CreateCollection&) = delete;
    CreateCollection& operator=(const CreateCollection&) = delete;

    BEGIN_VISITABLES(CreateCollection);
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<Timeseries>, timeseries);
    VISITABLE(std::optional<bsoncxx::array::value>, pipeline);
    VISITABLE(std::optional<bsoncxx::document::value>, validator);
    VISITABLE(std::optional<bsoncxx::document::value>, indexOptionDefaults);
    VISITABLE(std::optional<bsoncxx::document::value>, storageEngine);
    VISITABLE(std::optional<ChangeStream>, changeStreamPreAndPostImages);
    VISITABLE(std::optional<ClusteredIndex>, clusteredIndex);
    VISITABLE(std::string, viewOn);
    VISITABLE(std::optional<int64_t>, size);
    VISITABLE(std::optional<int64_t>, max);
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, expireAfterSeconds, {std::nullopt});
    VISITABLE_DIRECT_INIT(ValidationAction, validationAction, {ValidationAction::invalid});
    VISITABLE_DIRECT_INIT(ValidationLevel, validationLevel, {ValidationLevel::invalid});
    VISITABLE(std::optional<bool>, capped);
    END_VISITABLES;
  };
}
