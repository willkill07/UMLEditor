#pragma once

#include "model/relationship_type.hpp"
#include "utils/utils.hpp"

#include <nlohmann/json_fwd.hpp>

#include <compare>
#include <format>
#include <string>
#include <string_view>

namespace model {

class Relationship {
  std::string source_;
  std::string destination_;
  RelationshipType type_{RelationshipType::Inheritance};

  //NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Relationship const&);
  friend void from_json(nlohmann::json const&, Relationship&);
  //NOLINTEND(readability-identifier-naming)

public:
  ///
  /// @brief Create a relationship which has all arguments being validated
  ///
  /// @param source the name of the source class
  /// @param destination the name of the destination class
  /// @param type the type of the relationship
  /// @return error if validation failed else a constructed relationship
  ///
  [[nodiscard]] static Result<Relationship>
  From(std::string_view source, std::string_view destination, RelationshipType type);

  Relationship();

  [[nodiscard]] std::string const& Source() const noexcept;
  [[nodiscard]] std::string const& Destination() const noexcept;
  [[nodiscard]] RelationshipType Type() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(Relationship const&) const noexcept;

  [[nodiscard]] bool operator==(Relationship const&) const noexcept;

  ///
  /// @brief Change the source of this relationship
  ///
  /// @param new_source
  /// @return error IFF the source name wasn't validated
  ///
  [[nodiscard]] Result<void> ChangeSource(std::string_view new_source);

  ///
  /// @brief Change the destination of this relationship
  ///
  /// @param new_destination
  /// @return error IFF the destination name wasn't validated
  ///
  [[nodiscard]] Result<void> ChangeDestination(std::string_view new_destination);

  ///
  /// @brief Change the type of this relationship
  ///
  /// @param new_type
  ///
  void ChangeType(RelationshipType new_type);
};

} // namespace model

template <> struct std::formatter<model::Relationship> {
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    return ctx.begin();
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::Relationship const& obj, FormatContext& ctx) const {
    ctx.advance_to(std::format_to(ctx.out(), "{} -> {} ({})", obj.Source(), obj.Destination(), obj.Type()));
    return ctx.out();
  }
};
