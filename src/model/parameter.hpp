#pragma once

#include "utils/utils.hpp"

#include <nlohmann/json_fwd.hpp>

#include <compare>
#include <format>
#include <string>
#include <string_view>

namespace model {

class Parameter {
  std::string name_;
  std::string type_;

  //NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, model::Parameter const&);
  friend void from_json(nlohmann::json const&, model::Parameter&);
  //NOLINTEND(readability-identifier-naming)
public:
  ///
  /// @brief Create a parameter which has all arguments being validated
  ///
  /// @param name the name of the parameter
  /// @param type the type of the parameter
  /// @return error if validation failed else a constructed parameter
  ///
  [[nodiscard]] static Result<Parameter> From(std::string_view name, std::string_view type);

  ///
  /// @brief Construct a new Parameter object
  ///
  /// This only exists for the serde library in use
  ///
  ///Parameter();

  [[nodiscard]] std::string const& Name() const noexcept;
  [[nodiscard]] std::string const& Type() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(Parameter const&) const noexcept;

  [[nodiscard]] bool operator==(Parameter const&) const noexcept;

  ///
  /// @brief Rename a field
  ///
  /// @param name the new name
  /// @return error IFF validation of the new name failed
  ///
  [[nodiscard]] Result<void> Rename(std::string_view new_name);

  ///
  /// @brief Change type of the field
  ///
  /// @param new_type the new type
  /// @return error IFF validation of the new type failed
  ///
  [[nodiscard]] Result<void> ChangeType(std::string_view new_type);

  ///
  /// @brief Parsing function for Parameter
  ///
  /// @param str the string_view to parse
  /// @param start the starting index
  /// @return a pair holding the constructed parameter and the index as to where parsing concluded
  ///
  [[nodiscard]] static Result<std::pair<Parameter, std::size_t>> Parse(std::string_view str, std::size_t start = 0);

  ///
  /// @brief Parsing function for multiple Parameters
  ///
  /// @param str the string_view to parse
  /// @param start the starting index
  /// @return a pair holding the constructed parameters and the index as to where parsing concluded
  ///
  [[nodiscard]] static Result<std::pair<std::vector<Parameter>, std::size_t>> ParseMultiple(std::string_view str,
                                                                                            std::size_t start = 0);

  ///
  /// @brief Parsing function for Parameter from a string
  ///
  /// @param str the string_view to parse
  /// @return error IFF the string was a malformed parameter
  ///
  [[nodiscard]] static Result<Parameter> FromString(std::string_view str);

  ///
  /// @brief Parsing function for Parameters from a string
  ///
  /// @param str the string_view to parse
  /// @return error IFF the string was a malformed list of parameters
  ///
  [[nodiscard]] static Result<std::vector<Parameter>> MultipleFromString(std::string_view str);
};

} // namespace model

template <> struct std::formatter<model::Parameter> {
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
  auto format(model::Parameter const& obj, FormatContext& ctx) const {
    if (extended) {
      ctx.advance_to(std::format_to(ctx.out(), "{}: {}", obj.Name(), obj.Type()));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), "{}:{}", obj.Name(), obj.Type()));
    }
    return ctx.out();
  }
};
