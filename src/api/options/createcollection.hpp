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
      // The name of the field which contains the date in each time series document.
      VISITABLE(std::string, timeField);
      // The name of the field which contains metadata in each time series document.
      VISITABLE(std::string, metaField);
      // Sets the maximum time between timestamps in the same bucket.
      VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, bucketMaxSpanSeconds, {std::nullopt});
      // Sets the number of seconds to round down by when MongoDB sets the minimum timestamp for a new bucket.
      VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, bucketRoundingSeconds, {std::nullopt});
      // Set granularity to the value that most closely matches the time between consecutive incoming timestamps.
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
    // A document that expresses the write concern for the operation.
    VISITABLE(std::optional<WriteConcern>, writeConcern);
    // Specifies the default collation for the collection.
    VISITABLE(std::optional<Collation>, collation);
    VISITABLE(std::optional<Timeseries>, timeseries);
    // An array that consists of the aggregation pipeline stage(s).
    VISITABLE(std::optional<bsoncxx::array::value>, pipeline);
    // Allows users to specify validation rules or expressions for the collection.
    VISITABLE(std::optional<bsoncxx::document::value>, validator);
    // Allows users to specify a default configuration for indexes when creating a collection.
    VISITABLE(std::optional<bsoncxx::document::value>, indexOptionDefaults);
    // Allows users to specify configuration to the storage engine on a per-collection basis when creating a collection.
    VISITABLE(std::optional<bsoncxx::document::value>, storageEngine);
    VISITABLE(std::optional<ChangeStream>, changeStreamPreAndPostImages);
    VISITABLE(std::optional<ClusteredIndex>, clusteredIndex);
    // The name of the source collection or view from which to create a view.
    VISITABLE(std::string, viewOn);
    // Specify a maximum size in bytes for a capped collection.
    VISITABLE(std::optional<int64_t>, size);
    // The maximum number of documents allowed in the capped collection.
    VISITABLE(std::optional<int64_t>, max);
    // Specifies the seconds after which documents in a time series collection or clustered collection expire.
    VISITABLE_DIRECT_INIT(std::optional<std::chrono::seconds>, expireAfterSeconds, {std::nullopt});
    // Determines whether to error on invalid documents or just warn about the violations but allow invalid documents to be inserted.
    VISITABLE_DIRECT_INIT(ValidationAction, validationAction, {ValidationAction::invalid});
    // Determines how strictly MongoDB applies the validation rules to existing documents during an update.
    VISITABLE_DIRECT_INIT(ValidationLevel, validationLevel, {ValidationLevel::invalid});
    // To create a capped collection, specify `true`.
    VISITABLE(std::optional<bool>, capped);
    END_VISITABLES;
  };
}
