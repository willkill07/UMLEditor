#pragma once

#include "model/parameter.hpp"
#include "utils/utils.hpp"

#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace model {

class MethodSignature {
  std::string name_;
  std::vector<std::string> parameter_types_;

public:
  ///
  /// @brief Construct a new Method Signature object
  ///
  ///
  MethodSignature(std::string_view name, std::vector<std::string> parameters);

  ///
  /// @brief Parse a string as a method signature and return it
  ///
  /// @param str
  /// @return error if converting/parsing from a string failed in any way
  ///
  [[nodiscard]] static Result<MethodSignature> FromString(std::string_view str);

  [[nodiscard]] MethodSignature WithName(std::string_view name) const;
  [[nodiscard]] MethodSignature WithParameters(std::vector<std::string> parameters = {}) const;
  [[nodiscard]] MethodSignature WithParameters(std::vector<Parameter> const& parameters) const;
  [[nodiscard]] MethodSignature WithAddedParameter(std::string_view type) const;
  [[nodiscard]] MethodSignature WithoutParameter(std::size_t index) const;
  [[nodiscard]] MethodSignature WithParameterType(std::size_t index, std::string_view type) const;

  [[nodiscard]] std::string const& Name() const noexcept;
  [[nodiscard]] std::vector<std::string> const& ParameterTypes() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(MethodSignature const&) const noexcept;

  [[nodiscard]] bool operator==(MethodSignature const&) const noexcept;
};

} // namespace model

template <> struct std::formatter<model::MethodSignature> {
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    return ctx.begin();
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::MethodSignature const& obj, FormatContext& ctx) const {
    ctx.advance_to(std::format_to(ctx.out(), "{}(", obj.Name()));
    for (char const* sep = ""; std::string const& p : obj.ParameterTypes()) {
      ctx.advance_to(std::format_to(ctx.out(), "{}{}", std::exchange(sep, ","), p));
    }
    ctx.advance_to(std::format_to(ctx.out(), ")"));
    return ctx.out();
  }
};
