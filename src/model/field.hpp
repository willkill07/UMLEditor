#pragma once

#include "utils/utils.hpp"

#include <nlohmann/json_fwd.hpp>

#include <compare>
#include <format>
#include <string>
#include <string_view>

namespace model {

class Field {
  std::string name_;
  std::string type_;
  //NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Field const&);
  friend void from_json(nlohmann::json const&, Field&);
  //NOLINTEND(readability-identifier-naming)

public:
  ///
  /// @brief Create a field which has all arguments being validated
  ///
  /// @param name the name of the field
  /// @param type the type of the field
  /// @return error if validation failed else a constructed field
  ///
  static Result<Field> From(std::string_view name, std::string_view type);

  [[nodiscard]] std::string const& Name() const noexcept;
  [[nodiscard]] std::string const& Type() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(Field const&) const noexcept;

  [[nodiscard]] bool operator==(Field const&) const noexcept;

  ///
  /// @brief Rename a field
  ///
  /// @param name the new name
  /// @return error IFF validation of the new name failed
  ///
  [[nodiscard]] Result<void> Rename(std::string_view name);

  ///
  /// @brief Change type of the field
  ///
  /// @param new_type the new type
  /// @return error IFF validation of the new type failed
  ///
  [[nodiscard]] Result<void> ChangeType(std::string_view new_type);
};

} // namespace model

template <> struct std::formatter<model::Field> {
  bool extended{false};
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    auto pos = ctx.begin();
    while (pos != ctx.end() && *pos != '}') {
      if (*pos == ' ')
        extended = true;
      ++pos;
    }
    return pos;
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::Field const& obj, FormatContext& ctx) const {
    if (extended) {
      ctx.advance_to(std::format_to(ctx.out(), "{}: {}", obj.Name(), obj.Type()));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), "{}:{}", obj.Name(), obj.Type()));
    }
    return ctx.out();
  }
};
