//
// Created by Rakesh on 02/04/2026.
//

#include "../../src/common/util/date.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <print>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <catch2/reporters/catch_reporter_streaming_base.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/reverse.hpp>

namespace
{
  namespace phr
  {
    template <typename T>
    T create( std::string_view name )
    {
      auto t = T{};
      t.name = std::string{ std::string_view{ ranges::find_if( name, []( char a ) { return !std::isspace( a ); } ), name.end() } };
      t.sections.reserve( 16 );
      return t;
    }

    struct Section
    {
      Catch::Counts assertions;
      std::vector<Section> sections;
      Section* parent{ nullptr };
      std::string name;
      double duration{ 0.0 };
      bool printed{ false };
    };

    struct Suite
    {
      Catch::Counts assertions;
      std::vector<Section> sections;
      std::string name;
      decltype(std::chrono::steady_clock::now()) start{ std::chrono::steady_clock::now() };
      decltype(std::chrono::steady_clock::now()) end;

      [[nodiscard]] std::string filename() const
      {
        return std::format( "{}.html", boost::algorithm::replace_all_copy( name, " ", "-" ) );
      }

      [[nodiscard]] std::chrono::nanoseconds duration() const
      {
        return end - start;
      }
    };

    using std::operator ""sv;
    constexpr auto header = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<title>#TITLE#</title>
<style>
body {
  color: #222;
  background: #fff;
  font: 100% system-ui;
}
a {
  color: #0033cc;
}

@media (prefers-color-scheme: dark) {
  body {
    color: #eee;
    background: #121212;
  }

  body a {
    color: #809fff;
  }
}
b, p, div, span, a
{
  font-family: -apple-system, BlinkMacSystemFont, georgia, serif;
  font-size: small;
}
h1, h2, h3
{
  font-family: -apple-system, BlinkMacSystemFont, georgia, serif;
}
hr
{
  color: #ff9900;
}
a:link, a:visited
{
  font-family: -apple-system, BlinkMacSystemFont, georgia, serif;
  text-decoration: none;
  cursor: auto;
}

table
{
  border: 1px solid #1C6EA4;
  background-color: #EEEEEE;
  color: #121212;
  width: 100%;
  text-align: left;
  border-collapse: collapse;
}

table td, table th
{
  border: 1px solid #AAAAAA;
  font-family: -apple-system, BlinkMacSystemFont, georgia, serif;
  padding: 3px 2px;
}

table tbody td
{
  font-family: -apple-system, BlinkMacSystemFont, georgia, serif;
  font-size: 13px;
}

table tr:nth-child(even)
{
  background: #D0E4F5;
}

table thead
{
  background: #1C6EA4;
  background: -moz-linear-gradient(top, #5592bb 0%, #327cad 66%, #1C6EA4 100%);
  background: -webkit-linear-gradient(top, #5592bb 0%, #327cad 66%, #1C6EA4 100%);
  background: linear-gradient(to bottom, #5592bb 0%, #327cad 66%, #1C6EA4 100%);
  border-bottom: 2px solid #444444;
}

table thead th
{
  font-size: 15px;
  font-weight: bold;
  color: #FFFFFF;
  border-left: 2px solid #D0E4F5;
}

table thead th:first-child
{
  border-left: none;
}

table tfoot
{
  font-size: 14px;
  font-weight: bold;
  color: #FFFFFF;
  background: #D0E4F5;
  background: -moz-linear-gradient(top, #dcebf7 0%, #d4e6f6 66%, #D0E4F5 100%);
  background: -webkit-linear-gradient(top, #dcebf7 0%, #d4e6f6 66%, #D0E4F5 100%);
  background: linear-gradient(to bottom, #dcebf7 0%, #d4e6f6 66%, #D0E4F5 100%);
  border-top: 2px solid #444444;
}

table tfoot td
{
  font-size: 14px;
}

table tfoot .links
{
  text-align: right;
}

table tfoot .links a
{
  display: inline-block;
  background: #1C6EA4;
  color: #FFFFFF;
  padding: 2px 8px;
  border-radius: 5px;
}

ol
{
  list-style: none;
  counter-reset: item;
}
li
{
  counter-increment: item;
  margin-bottom: 5px;
}
li:before
{
  margin-right: 10px;
  content: counter(item);
  background: lightblue;
  border-radius: 100%;
  color: white;
  width: 1.2em;
  text-align: center;
  display: inline-block;
}
</style>
</head>
<body>
)html"sv;

    void createIndex( std::span<const Suite> suites, std::filesystem::path path,
      std::chrono::time_point<std::chrono::system_clock> testStartTime )
    {
      path /= "index.html";
      auto file = std::ofstream{ path.c_str(), std::ios::trunc };
      if ( !file.is_open() )
      {
        std::println( "Failed to open file {}", path.c_str() );
        return;
      }

      auto head = std::string{ header };
      boost::replace_all( head, "#TITLE#", "Test Report" );
      file.write( head.data(), static_cast<std::streamsize>( head.size() ) );

      static constexpr auto preamble = R"html(
<h2>Test Results</h2>
<div><a href='results.html'>Aggregate Results</a></div>
<div><a href='summary.html'>Test Summary</a></div>
<div>Links to individual test results below...</div>
<ol>
)html"sv;
      file.write( preamble.data(), preamble.size() );

      const auto symbol = []( const Suite& suite ) -> std::string_view
      {
        return suite.assertions.allPassed() ? "&#x2713;" : "&times;";
      };

      const auto colour = []( const Suite& suite ) -> std::string_view
      {
        return suite.assertions.allPassed() ? "green" : "red";
      };

      for ( const auto& suite : suites )
      {
        auto line = std::format( "<li><span style='color: {}'>{}</span> <a href='{}'>{}</a></li>",
          colour( suite ), symbol( suite ), suite.filename(), suite.name );
        file.write( line.data(), static_cast<std::streamsize>( line.size() ) );
      }

      auto line = std::format( "</ol>\n<div>Test suite start time: {}</div>\n", spt::util::isoDateMicros( testStartTime ) );
      file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

      static constexpr auto footer = "</body>\n</html>\n"sv;
      file.write( footer.data(), footer.size() );
    }

    void createSummary( std::span<const Suite> suites, std::filesystem::path path )
    {
      path /= "summary.html";
      auto file = std::ofstream{ path.c_str(), std::ios::trunc };
      if ( !file.is_open() )
      {
        std::println( "Failed to open file {}", path.c_str() );
        return;
      }

      auto head = std::string{ header };
      boost::replace_all( head, "#TITLE#", "Test Report Summary" );
      file.write( head.data(), static_cast<std::streamsize>( head.size() ) );

      static constexpr auto preamble = R"html(
<table>
<thead>
<tr>
<th>Suite</th>
<th>Passed</th>
<th>Failed</th>
<th>FailedButOk</th>
<th>Skipped</th>
<th>Time (seconds)</th>
</tr>
</thead>
<tbody>
)html"sv;
      file.write( preamble.data(), preamble.size() );

      auto tp = std::size_t{ 0 };
      auto tf = std::size_t{ 0 };
      auto tfo = std::size_t{ 0 };
      auto ts = std::size_t{ 0 };
      auto total = std::chrono::nanoseconds{ 0 };

      for ( const auto& suite : suites )
      {
        tp += suite.assertions.passed;
        tf += suite.assertions.failed;
        tfo += suite.assertions.failedButOk;
        ts += suite.assertions.skipped;
        total += suite.duration();

        auto line = std::format( R"html(<tr>
<td>{}</td>
<td><span style='color: green'>{}</span></td>
<td><span style='color: red'>{}</span></td>
<td><span style='color: orange'>{}</span></td>
<td><span style='color: purple'>{}</span></td>
<td>{}</td>
</tr>
)html",
          suite.name, suite.assertions.passed, suite.assertions.failed,
          suite.assertions.failedButOk, suite.assertions.skipped, std::chrono::duration_cast<std::chrono::duration<double>>( suite.duration() ).count() );
        file.write( line.data(), static_cast<std::streamsize>( line.size() ) );
      }

      auto line = std::format( R"html(<tr>
<td><strong>Total</strong></td>
<td><span style='font-weight: bold; color: green'>{}</span></td>
<td><span style='font-weight: bold; color: red'>{}</span></td>
<td><span style='font-weight: bold; color: orange'>{}</span></td>
<td><span style='font-weight: bold; color: purple'>{}</span></td>
<td><strong>{}</strong></td>
</tr>)html",
        tp, tf, tfo, ts, std::chrono::duration_cast<std::chrono::duration<double>>( total ).count() );
      file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

      static constexpr auto footer = R"html(
</tbody>
</table>
</body>
</html>
)html"sv;
      file.write( footer.data(), footer.size() );
    }

    void write( const Section& section, std::ofstream& file )
    {
      static const auto symbol = []( const Section& section ) -> std::string_view
      {
        return section.assertions.allPassed() ? "&#x2713;" : "&times;";
      };

      static const auto colour = []( const Section& section ) -> std::string_view
      {
        return section.assertions.allPassed() ? "green" : "red";
      };

      file.write( "<li>", 4 );

      auto line = std::format( "<span style='color: {}'>{}</span> <span style='color: gray'>{} ({} seconds)</span>",
        colour( section ), symbol( section ), section.name, section.duration );
      file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

      line = std::format( "<div>Assertions - <span style='color: green'>Passed: {}</span> <span style='color: red'>Failed: {}</span> <span style='color: orange'>FailedButOk: {}</span> <span style='color: purple'>Skipped: {}</span></div>",
        section.assertions.passed, section.assertions.failed, section.assertions.failedButOk, section.assertions.skipped );
      file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

      file.write( "<ol>", 4 );
      for ( const auto& sec : section.sections ) write( sec, file );
      file.write( "</ol>", 5 );

      file.write( "</li>", 5 );
    }

    void write( const Suite& suite, std::filesystem::path path )
    {
      path /= suite.filename();
      auto file = std::ofstream{ path.c_str(), std::ios::trunc };
      if ( !file.is_open() )
      {
        std::println( "Failed to open file {}", path.c_str() );
        return;
      }

      auto head = std::string{ header };
      boost::replace_all( head, "#TITLE#", suite.name );
      file.write( head.data(), static_cast<std::streamsize>( head.size() ) );

      auto line = std::format( "<h2>{}</h2>\n<ol>", suite.name );
      file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

      for ( const auto& section : suite.sections ) write( section, file );

      static constexpr auto footer = R"html(
</ol>
</body>
</html>
)html"sv;
      file.write( footer.data(), footer.size() );
    }

    void createResults( std::span<const Suite> suites, std::filesystem::path path )
    {
      path /= "results.html";
      auto file = std::ofstream{ path.c_str(), std::ios::trunc };
      if ( !file.is_open() )
      {
        std::println( "Failed to open file {}", path.c_str() );
        return;
      }

      auto head = std::string{ header };
      boost::replace_all( head, "#TITLE#", "Aggregated Test Report" );
      file.write( head.data(), static_cast<std::streamsize>( head.size() ) );

      file.write( "<ol>\n", 5 );
      for ( const auto& suite : suites )
      {
        auto line = std::format( "<li><strong>{}</strong>\n", suite.name );
        file.write( line.data(), static_cast<std::streamsize>( line.size() ) );

        file.write( "<ol>\n", 5 );
        for ( const auto& section : suite.sections ) write( section, file );

        line = "</ol>\n</li>\n<hr/>";
        file.write( line.data(), static_cast<std::streamsize>( line.size() ) );
      }

      static constexpr auto footer = R"html(
</ol>
</body>
</html>
)html"sv;
      file.write( footer.data(), footer.size() );
    }

    void generate( std::span<const Suite> suites, std::string_view directory,
      std::chrono::time_point<std::chrono::system_clock> testStartTime )
    {
      auto start = std::chrono::steady_clock::now();
      std::println( "Generating HTML report for {} suites", suites.size() );
      std::filesystem::path path{ directory };

      if ( !std::filesystem::exists( path ) )
      {
        if ( !std::filesystem::create_directory( path ) )
        {
          std::println( "Failed to create directory {}", path.c_str() );
          return;
        }
      }

      createIndex( suites, path, testStartTime );
      createSummary( suites, path );
      createResults( suites, path );

      for( const auto& suite : suites ) write( suite, path );

      std::println( "\033[1;34m  Generated HTML reports in - \033[0m\033[1m{} \033[0m\033[1;34mseconds\033[0m", std::chrono::duration_cast<std::chrono::duration<double>>( std::chrono::steady_clock::now() - start ).count() );
    }
  }
}

namespace spt::test::reporter
{
  struct HtmlReporter : Catch::StreamingReporterBase
  {
    using StreamingReporterBase::StreamingReporterBase;

    [[maybe_unused]] static std::string getDescription() { return "HTML reporter"; }

    void testRunStarting( const Catch::TestRunInfo& info ) override
    {
      StreamingReporterBase::testRunStarting( info );
      suites.reserve( 16 );
      seed = Catch::getSeed();
      start = std::chrono::steady_clock::now();
      startTime = std::chrono::system_clock::now();
    }

    void testCaseStarting( const Catch::TestCaseInfo& info ) override
    {
      StreamingReporterBase::testCaseStarting( info );
      currentSection = nullptr;
      suites.push_back( phr::create<phr::Suite>( info.name ) );
      std::println( "\033[1;34m{}\033[0m", info.name );
    }

    void sectionStarting( const Catch::SectionInfo& info ) override
    {
      StreamingReporterBase::sectionStarting( info );
      auto test = phr::create<phr::Section>( info.name );
      test.parent = currentSection;

      if ( test.name == suites.back().name ) return;

      const auto add = [this]( std::vector<phr::Section>& sections, phr::Section&& sec )
      {
        if ( auto iter = ranges::find_if( sections, [&sec]( const auto& tc ) { return tc.name == sec.name; } );
          iter == ranges::end( sections ) )
        {
          sections.push_back( std::move( sec ) );
          currentSection = &sections.back();
        }
        else
        {
          currentSection = iter.base();
        }
      };

      add( currentSection ? currentSection->sections : suites.back().sections, std::move( test ) );
    }

    void assertionEnded( const Catch::AssertionStats& stats ) override
    {
      if ( !stats.assertionResult.isOk() )
      {
        std::println( "\033[1;31mAssertion '{}' failed: {} at {}:{}\033[0m",
          stats.assertionResult.getExpression(), stats.assertionResult.getMessage().data(),
          stats.assertionResult.getSourceInfo().file, stats.assertionResult.getSourceInfo().line );
      }
    }

    void sectionEnded( const Catch::SectionStats& stats ) override
    {
      StreamingReporterBase::sectionEnded( stats );

      if ( !currentSection ) return;
      if ( const auto sec = phr::create<phr::Section>( stats.sectionInfo.name ); sec.name == suites.back().name ) return;

      currentSection->assertions.passed += stats.assertions.passed;
      currentSection->assertions.failed += stats.assertions.failed;
      currentSection->assertions.failedButOk += stats.assertions.failedButOk;
      currentSection->assertions.skipped += stats.assertions.skipped;
      currentSection->duration += stats.durationInSeconds;

      const auto indent = []( const phr::Section& sec ) -> std::string
      {
        auto indent = std::string{};
        indent.reserve( 16 );
        indent.append( 2, ' ' );

        auto root = sec.parent;
        while ( root )
        {
          indent.append( 2, ' ' );
          root = root->parent;
        }

        return indent;
      };

      const auto style = []( const phr::Section& sec ) -> std::string
      {
        if ( sec.assertions.allPassed() ) return "\033[36m";
        return "\033[33m";
      };

      const auto symbol = [] ( const phr::Section& sec ) -> std::string
      {
        return sec.assertions.allPassed() ? "✅" : "❌";
      };

      auto messages = std::vector<std::string>{};
      messages.reserve( 8 );
      if ( !currentSection->printed )
      {
        messages.push_back( std::format( "{}{}{} {}\033[0m", indent( *currentSection ), style( *currentSection ), symbol( *currentSection ), currentSection->name ) );
        currentSection->printed = true;
      }

      auto root = currentSection->parent;
      while ( root && !root->printed )
      {
        messages.push_back( std::format( "{}{}{} {}", indent( *root ), style( *root ), symbol( *root ), root->name ) );
        root->printed = true;
        root = root->parent;
      }

      for ( const auto& msg : messages | ranges::views::reverse ) std::println( "{}", msg );

      currentSection = currentSection->parent;
    }

    void testCaseEnded( const Catch::TestCaseStats& stats ) override
    {
      currentSection = nullptr;
      if ( !stats.testInfo ) return;

      auto& suite = suites.back();
      suite.assertions.passed += stats.totals.assertions.passed;
      suite.assertions.failed += stats.totals.assertions.failed;
      suite.assertions.failedButOk += stats.totals.assertions.failedButOk;
      suite.assertions.skipped += stats.totals.assertions.skipped;
      suite.end = std::chrono::steady_clock::now();

      std::println( "\033[1;34m{} \033[0m\033[1m({} seconds)\033[0m", suite.name, std::chrono::duration_cast<std::chrono::duration<double>>( suite.duration() ).count() );
      std::println( "\033[1;34m  Assertions - \033[0m\033[1;32mPassed: {}; \033[0m\033[1;31mFailed: {}, \033[0m\033[1;33mFailedOk: {}, \033[0m\033[1;35mSkipped: {}, \033[0m\033[1mTotal: {}\033[0m",
        suite.assertions.passed, suite.assertions.failed, suite.assertions.failedButOk,
        suite.assertions.skipped, suite.assertions.total() );
    }

    void testRunEnded( const Catch::TestRunStats& stats ) override
    {
      StreamingReporterBase::testRunEnded( stats );

      std::println( "\033[1;34m{}\033[0m", stats.runInfo.name.data() );
      std::println( "\033[1;34m  Test Cases - \033[0m\033[1;32mPassed: {}; \033[0m\033[1;31mFailed: {}, \033[0m\033[1;33mFailedOk: {}, \033[0m\033[1;35mSkipped: {}, \033[0m\033[1mTotal: {}\033[0m",
        stats.totals.testCases.passed, stats.totals.testCases.failed, stats.totals.testCases.failedButOk,
        stats.totals.testCases.skipped, stats.totals.testCases.total() );
      std::println( "\033[1;34m  Assertions -  \033[0m\033[1;32mPassed: {}; \033[0m\033[1;31mFailed: {}, \033[0m\033[1;33mFailedOk: {}, \033[0m\033[1;35mSkipped: {}; \033[0m\033[1mTotal: {}\033[0m",
        stats.totals.assertions.passed, stats.totals.assertions.failed, stats.totals.assertions.failedButOk,
        stats.totals.assertions.skipped, stats.totals.assertions.total() );

      std::println( "\033[1;34m  Duration - \033[0m\033[1m{} \033[0m\033[1;34mseconds\033[0m", std::chrono::duration_cast<std::chrono::duration<double>>( std::chrono::steady_clock::now() - start ).count() );
      phr::generate( suites, "test-results", startTime );
    }

  private:
    std::vector<phr::Suite> suites;
    phr::Section* currentSection{ nullptr };
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    uint32_t seed{ 0 };
  };
}

CATCH_REGISTER_REPORTER( "html", spt::test::reporter::HtmlReporter )