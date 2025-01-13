#pragma once

#include <nlohmann/json_fwd.hpp>

#include <format>
#include <utility>

#include "utils/utils.hpp"

namespace model {

enum class RelationshipType : std::uint8_t { Aggregation, Composition, Inheritance, Realization };

// NOLINTBEGIN(readability-identifier-naming)
void to_json(nlohmann::json&, RelationshipType);
void from_json(nlohmann::json const&, RelationshipType&);
// NOLINTEND(readability-identifier-naming)

[[nodiscard]] Result<RelationshipType> RelationshipTypeFromString(std::string_view);

} // namespace model

template <> struct std::formatter<model::RelationshipType> {
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    return ctx.begin();
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::RelationshipType const& obj, FormatContext& ctx) const {
    constexpr static std::array<std::string_view, 4> names{"Aggregation", "Composition", "Inheritance", "Realization"};
    ctx.advance_to(std::format_to(ctx.out(), "{}", names[static_cast<std::size_t>(std::to_underlying(obj))]));
    return ctx.out();
  }
};
