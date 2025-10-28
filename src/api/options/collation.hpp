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
  struct Collation
  {
    enum class CaseFirst : uint_fast8_t { upper, lower, off };
    enum class MaxVariable : uint_fast8_t { punct, space };

    explicit Collation( bsoncxx::document::view doc ) { util::unmarshall( *this, doc ); }

    Collation() = default;
    ~Collation() = default;
    Collation(Collation&&) = default;
    Collation& operator=(Collation&&) = default;
    bool operator==(const Collation&) const = default;

    Collation(const Collation&) = delete;
    Collation& operator=(const Collation&) = delete;

    BEGIN_VISITABLES(Collation);
    // The ICU locale.
    VISITABLE(std::string, locale);
    // A field that determines sort order of case differences during tertiary level comparisons.
    VISITABLE_DIRECT_INIT(CaseFirst, caseFirst, {CaseFirst::off});
    // Field that determines whether collation should consider whitespace and punctuation as base characters for purposes of comparison.
    VISITABLE(std::string, alternate);
    // Field that determines up to which characters are considered ignorable when alternate: "shifted". Has no effect if alternate: "non-ignorable"
    VISITABLE(std::optional<MaxVariable>, maxVariable);
    // The level of comparison to perform. Corresponds to ICU Comparison Levels
    VISITABLE(std::optional<int32_t>, strength);
    // Flag that determines whether to include case comparison at strength level 1 or 2.
    VISITABLE(std::optional<bool>, caseLevel);
    // Flag that determines whether to compare numeric strings as numbers or as strings.
    VISITABLE(std::optional<bool>, numericOrdering);
    // Flag that determines whether strings with diacritics sort from back of the string, such as with some French dictionary ordering.
    VISITABLE(std::optional<bool>, backwards);
    // Flag that determines whether to check if text requires normalisation and to perform normalisation.
    VISITABLE(std::optional<bool>, normalization);
    END_VISITABLES;
  };
}