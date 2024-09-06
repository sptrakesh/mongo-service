//
// Created by Rakesh on 24/05/2023.
//

#pragma once

#include "../visit_struct/fully_visitable.hpp"

namespace spt::util
{
  /**
   * A visitable concept.  Any default constructable structure that is also visitable.
   * @tparam T The type that is considered visitable.
   */
  template <typename T>
  concept Visitable = requires( T t )
  {
    std::is_default_constructible<T>{};
    visit_struct::traits::is_visitable<T>{};
  };

  /**
   * A concept that restricts a type to not being a scoped enumeration.
   */
  template <typename T>
  concept NotEnumeration = not std::is_enum_v<T>;
}