//
// Created by Rakesh on 07/03/2025.
//

#pragma once

#include <chrono>
#include <map>
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
}
