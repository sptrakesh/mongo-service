//
// Created by Rakesh on 2019-05-26.
//

#pragma once

#include "apmrecord.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace spt::ilp
{
  /**
   * Builder for composing a batch of records to transmit over ILP to a TSDB.
   */
  struct Builder
  {
    Builder() = default;
    ~Builder() = default;

    Builder(Builder&&) = delete;
    Builder& operator=(Builder&&) = delete;
    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    /**
     * Start a new timeseries record.  Each record is sent as a single line in ILP.
     * @param name The name of the timeseries.  This generally translates into a `table` in the TSDB.
     * @return This builder instance for method chaining.
     */
    Builder& startRecord( std::string_view name );

    /**
     * Add a tag to the current record.
     * @param key The key/name of the tag to send as part of the record.
     * @param value The value for the tag.
     * @return This builder instance for method chaining.
     */
    Builder& addTag( std::string_view key, std::string_view value );

    /**
     * Add a boolean value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The boolean value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, bool value );

    /**
     * Add a 32-bit integer value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The integer value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, int32_t value );

    /**
     * Add a unsigned 32-bit integer value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The integer value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, uint32_t value );

    /**
     * Add a 64-bit integer value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The integer value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, int64_t value );

    /**
     * Add a unsigned 64-bit integer value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The integer value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, uint64_t value );

    /**
     * Add a 32-bit floating point value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The floating point value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, float value );

    /**
     * Add a 64-bit floating point value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The floating point value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, double value );

    /**
     * Add a string value to the current record.
     * @param key The key/name of the value to send as part of the record.
     * @param value The string value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, std::string_view value );

    using DateTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
    using DateTimeMs = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
    using DateTimeNs = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;

    /**
     * Add a date-time value to the current record.  Date-times are serialised as the number of microseconds since UNIX epoch.
     * @param key The key/name of the value to send as part of the record.
     * @param value The date-time value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, DateTime value );

    /**
     * Add a date-time value to the current record.  Date-times are serialised as the number of microseconds since UNIX epoch.
     * @param key The key/name of the value to send as part of the record.
     * @param value The date-time value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, DateTimeMs value );

    /**
     * Add a date-time value to the current record.  Date-times are serialised as the number of nanoseconds since UNIX epoch.
     * @param key The key/name of the value to send as part of the record.
     * @param value The date-time value to send.
     * @return This builder instance for method chaining.
     */
    Builder& addValue( std::string_view key, DateTimeNs value );

    /**
     * Set the timestamp for the current record.
     * @param value The timestamp for the current record.
     * @return This builder instance for method chaining.
     */
    Builder& timestamp( std::chrono::nanoseconds value );

    /**
     * End the current record.  The record is serialised as a single line into the batch of records.
     * @return This builder instance for method chaining.
     */
    Builder& endRecord();

    /**
     * Create records in the batch that represents the APM record and its child processes.
     * @param name The name of the timeseries to record the APM record.
     * @param record The APM record to send over ILP.
     * @return This builder instance for method chaining.
     */
    Builder& add( std::string_view name, const APMRecord& record );

    /**
     * Generate the message with lines representing the records in this batch.  The builder should not be reused.
     * @return The string which contains lines for each record that was included in this batch.
     */
    [[nodiscard]] std::string finish();

  private:
    struct Record
    {
      explicit Record( std::string_view name ) : name{ name } {}

      ~Record() = default;
      Record(Record&&) = default;
      Record& operator=(Record&&) = default;

      Record(const Record&) = delete;
      Record& operator=(const Record&) = delete;

      std::string name;
      std::string tags;
      std::string value;
      std::chrono::nanoseconds timestamp{ std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() ) };
    };

    std::string value;
    std::optional<Record> record{ std::nullopt };
  };
}

