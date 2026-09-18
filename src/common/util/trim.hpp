//
// Created by Rakesh on 17/09/2026.
//

#pragma once

#include <algorithm>
#include <ranges>
#include <string_view>

namespace spt::util
{
  /**
   * Trim/skip leading unwanted characters from a string view.
   * @tparam T The predicate function to apply
   * @param view The string view to trim leading characters from
   * @param pred The predicate function to apply to skip unwanted characters
   * @return The view with unwanted leading characters skipped
   */
  template <typename T = bool (*)(char)>
  [[nodiscard]] constexpr std::string_view trim_left( std::string_view view, T pred = [](char a) { return !std::isspace(a); } )
  {
    return { std::ranges::find_if( view, pred ), view.end() };
  }

  /**
   * Trim/skip trailing unwanted characters from a string view.
   * @tparam T The predicate function to apply
   * @param view The string view to trim trailing characters from
   * @param pred The predicate function to apply to skip unwanted characters
   * @return The view with unwanted trailing characters skipped
   */
  template <typename T = bool (*)(char)>
  [[nodiscard]] constexpr std::string_view trim_right( std::string_view view, T pred = [](char a) { return !std::isspace(a); } )
  {
    return { view.begin(), std::find_if( view.rbegin(),  view.rend(), pred ).base() };
  }

  /**
   * Trim/skip leading and trailing unwanted characters from a string view.
   * @tparam T The predicate function to apply
   * @param view The string view to trim unwanted leading and trailing characters from
   * @param pred The predicate function to apply to skip unwanted characters
   * @return The view with unwanted leading and trailing characters skipped
   */
  template <typename T = bool (*)(char)>
  [[nodiscard]] constexpr std::string_view trim( std::string_view view, T pred = [](char a) { return !std::isspace(a); } )
  {
    return trim_left( trim_right( view, pred ), pred );
  }
}