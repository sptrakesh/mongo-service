//
// Created by Rakesh on 07/03/2025.
//

#pragma once

#include <chrono>
#include <format>
#include <map>
#include <source_location>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace spt::ilp
{
  /**
   * A general purpose structure for capturing Application Performance Monitoring (APM) data.
   *
   * The top level structure captures information for a specific business process.  The `processes`
   * array captures information that pertains to the various functions, steps etc. that are executed
   * as part of the parent process.
   *
   * The parent and all related child process information can then be stored in a timeseries database via ILP.
   */
  struct APMRecord
  {
    using DateTime = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;
    /// Allowed value types in the `values` map.
    using Value = std::variant<bool, int64_t, uint64_t, double, std::string>;

    /**
     * Representation of a task/process executed as part of a business process.
     */
    struct Process
    {
      /// Enumeration of process types.
      enum class Type : std::uint8_t
      {
        /// The process is executed as a function
        Function,
        /// The process is executed as a step/instruction within a function
        Step,
        /// Some other type
        Other
      };

      explicit Process( Type type ) : type( type ) {}
      Process() = default;
      ~Process() = default;
      Process( Process&& ) = default;
      Process& operator=( Process&& ) = default;

      Process( const Process& ) = delete;
      Process& operator=( const Process& ) = delete;

      /// Tags that will be sent via ILP.  Do not include application or id.
      std::map<std::string, std::string, std::less<>> tags;
      /// Values that will be sent via ILP.  Do not include duration.
      std::map<std::string, Value, std::less<>> values;
      /// The timestamp at which this process was initiated.
      DateTime timestamp{ std::chrono::high_resolution_clock::now() };
      /// The duration for completing the process.  This is sent as a value over ILP.
      /// Note: Do not include a duration value in the `values` map.
      std::chrono::nanoseconds duration{ 0 };
      /// The type of the process.  Default `Other`
      Type type{ Type::Other };
    };

    /**
     * Create a new APM record that will be sent to a TSDB over ILP.
     * @param id The unique id assigned to each invocation of the business process.
     */
    explicit APMRecord( std::string_view id ) : id{ id } {}
    ~APMRecord() = default;
    APMRecord( APMRecord&& ) = default;
    APMRecord& operator=( APMRecord&& ) = default;

    APMRecord( const APMRecord& ) = delete;
    APMRecord& operator=( const APMRecord& ) = delete;

    std::vector<Process> processes;
    /// Tags that will be sent via ILP.  Do not include application or id.
    std::map<std::string, std::string, std::less<>> tags;
    /// Values that will be sent via ILP.  Do not include duration.
    std::map<std::string, Value, std::less<>> values;
    /// A unique id assigned to each execution of the business process.
    std::string id;
    /// A tag used to identify the application/service that executes the business process.
    /// Note: do not add an application tag to the `tags` map.
    std::string application;
    /// The timestamp at which this business process was initiated.
    DateTime timestamp{ std::chrono::high_resolution_clock::now() };
    /// The duration for completing the process.  This should in general be greater than the sum
    /// of all durations for child processes.  This is sent as a value via ILP.
    /// Note: Do not include a duration value in the `values` map.
    std::chrono::nanoseconds duration{ 0 };
  };

  /// Concept that defines an APM record or process.  A structure that has a `timestamp` and `duration` of appropriate type.
  template <typename T>
  concept Record = requires( T t )
  {
    std::is_same_v<decltype(t.timestamp), std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>>;
    std::is_same_v<decltype(t.duration), std::chrono::nanoseconds>;
  };

  /**
   * Utility function to create a new APM record.
   * @param id The id to assign to the record being created
   * @param application The name of the application for which the record is being created.
   * @param type The type to assign to the first process to add to the record.
   * @param size The number of `processes` to reserve space for.
   * @param loc The location at which the record is to be created.
   * @return The newly created APM record.
   */
  APMRecord createAPMRecord( std::string_view id, std::string_view application, APMRecord::Process::Type type,
    std::size_t size, std::source_location loc = std::source_location::current() );

  /**
   * Add a new process of specified type to the APM record.
   * @param apm The APM record to which new Process is to be added.
   * @param type The type of process to add.
   * @param loc The location at which the record is to be created.
   * @return Reference to the newly inserted process.
   */
  APMRecord::Process& addProcess( APMRecord& apm, APMRecord::Process::Type type, std::source_location loc = std::source_location::current() );

  /**
   *
   * @param apm The APM record to which a caught exception is added as a `Step`.
   * @param ex The exception that was caught.
   * @param prefix The prefix to add to the `std::exception::what` function value in the step.
   * @param loc The location at which the record is to be created.
   * @return Reference to the newly inserted process.
   */
  APMRecord::Process& addException( APMRecord& apm, const std::exception& ex, std::string_view prefix,
    std::source_location loc = std::source_location::current() );

  /**
   * Set the duration for the record based on current timestamp.  Duration is difference between the record/process
   * `timestamp` and current timestamp.
   * @tparam T The APM record or process type.
   * @param record The record or process for which the `duration` is to be set.
   */
  template <Record T>
  void setDuration( T& record )
  {
    if ( record.duration.count() > 0 ) return;
    record.duration = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now() - record.timestamp );
  }

  /**
   * Add the current function as the calling function for the specified record.
   * @tparam T The type of record (APMRecord or Process).
   * @param record The record to which the calling function is to be added as a *value*.
   * @param loc The source location from which the caller is derived.
   * @param tagName The _prefix_ for the names of the values with source location information to assign.
   */
  template <Record T>
  void addCurrentFunction( T& record, const std::source_location loc = std::source_location::current(), std::string_view tagName = "caller" )
  {
    record.values.try_emplace( std::format( "{}_file", tagName ), loc.file_name() );
    record.values.try_emplace( std::format( "{}_line", tagName ), static_cast<uint64_t>( loc.line() ) );
    if ( auto fn = std::string{ loc.function_name() }; !fn.empty() ) record.values.try_emplace( std::format( "{}_function", tagName ), std::move( fn ) );
  }
}