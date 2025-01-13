#pragma once

#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "utils/utils.hpp"

#include <nlohmann/json_fwd.hpp>

#include <compare>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace model {

class Method {
  std::string name_;
  std::string return_type_;
  std::vector<Parameter> parameters_;

  //NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Method const&);
  friend void from_json(nlohmann::json const&, Method&);
  //NOLINTEND(readability-identifier-naming)

public:
  ///
  /// @brief Create a method which has all arguments being validated
  ///
  /// @param name the name of the method
  /// @param return_type the return type of the method
  /// @param parameters the parameters of the method
  /// @return error if validation failed else a constructed method
  ///
  [[nodiscard]] static Result<Method>
  From(std::string_view name, std::string_view return_type, std::vector<Parameter> parameters);

  [[nodiscard]] std::string const& Name() const noexcept;
  [[nodiscard]] std::string const& ReturnType() const noexcept;
  [[nodiscard]] std::vector<Parameter> const& Parameters() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(Method const&) const noexcept;

  [[nodiscard]] bool operator==(Method const&) const noexcept;

  [[nodiscard]] bool operator==(MethodSignature const& sig) const noexcept;

  ///
  /// @brief Get a string representation of the method's signature (name + parameter types)
  ///
  /// @return the signature of this method
  ///
  [[nodiscard]] std::string ToSignatureString() const;

  ///
  /// @brief Get an iterator to a parameter
  ///
  /// @param parameter_name
  /// @return error if the name is malformed or if the parameter doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Parameter>::iterator> GetParameter(std::string_view parameter_name);

  ///
  /// @brief Get an iterator to a parameter
  ///
  /// @param parameter_name
  /// @return error if the name is malformed or if the parameter doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Parameter>::const_iterator>
  GetReadOnlyParameter(std::string_view parameter_name) const;

  ///
  /// @brief Rename the method
  ///
  /// @param name
  /// @return error if the name is invalid
  ///
  [[nodiscard]] Result<void> Rename(std::string_view name);

  ///
  /// @brief Change the return type of the method
  ///
  /// @param new_type
  /// @return error if the new type is invalid
  ///
  [[nodiscard]] Result<void> ChangeReturnType(std::string_view new_type);

  ///
  /// @brief Add a parameter to the method
  ///
  /// @param parameter_name
  /// @param parameter_type
  /// @return error if either the name or type are invalid
  ///
  [[nodiscard]] Result<void> AddParameter(std::string_view parameter_name, std::string_view parameter_type);

  ///
  /// @brief Remove a parameter from the method
  ///
  /// @param iter
  /// @return error if removing failed
  ///
  [[nodiscard]] Result<void> RemoveParameter(std::vector<Parameter>::const_iterator iter);

  ///
  /// @brief Clear all parameters from the method
  ///
  /// @return error if clearing parameters failed
  ///
  [[nodiscard]] Result<void> ClearParameters();

  ///
  /// @brief Rename a method's parameter
  ///
  /// @param parameter_name
  /// @param new_name
  /// @return error if either the existing name or new name was invalid
  ///
  [[nodiscard]] Result<void> RenameParameter(std::string_view parameter_name, std::string_view new_name);

  ///
  /// @brief Change all parameters of the method
  ///
  /// @param parameters
  /// @return error if any of the new parameters were malformed
  ///
  [[nodiscard]] Result<void> ChangeParameters(std::vector<Parameter> parameters);

  ///
  /// @brief Get the zero-based index of the given iterator
  ///
  /// @return std::size_t
  ///
  [[nodiscard]] std::size_t GetParameterIndex(std::vector<model::Parameter>::const_iterator) const;

  ///
  /// @brief Parse a method from a string
  ///
  /// @param str
  /// @return error IFF there was an error encountered during conversion/parsing
  ///
  [[nodiscard]] static Result<Method> FromString(std::string_view str);
};

} // namespace model

template <> struct std::formatter<model::Method> {
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
  auto format(model::Method const& obj, FormatContext& ctx) const {
    ctx.advance_to(std::format_to(ctx.out(), "{}", obj.Name()));
    for (char const* sep = "("; model::Parameter const& p : obj.Parameters()) {
      if (extended) {
        ctx.advance_to(std::format_to(ctx.out(), "{}{: }", std::exchange(sep, ", "), p));
      } else {
        ctx.advance_to(std::format_to(ctx.out(), "{}{}", std::exchange(sep, ","), p));
      }
    }
    ctx.advance_to(std::format_to(ctx.out(), "){}{}", (extended ? " -> " : "->"), obj.ReturnType()));
    return ctx.out();
  }
};
