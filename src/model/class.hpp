#pragma once

#include "model/field.hpp"
#include "model/method.hpp"
#include "model/method_signature.hpp"

#include <nlohmann/json_fwd.hpp>

#include <algorithm>
#include <compare>
#include <format>
#include <ranges>
#include <string>
#include <vector>

namespace model {

///
/// @brief A basic point class used to represent a location
///
///
struct Point {
  int x{0};
  int y{0};

  // NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Point const&);
  friend void from_json(nlohmann::json const&, Point&);
  // NOLINTEND(readability-identifier-naming)

  constexpr auto operator<=>(Point const&) const noexcept = default;
};

class Class {
  std::string name_;
  std::vector<Field> fields_;
  std::vector<Method> methods_;
  Point position_{};

  // NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Class const&);
  friend void from_json(nlohmann::json const&, Class&);
  // NOLINTEND(readability-identifier-naming)

public:
  ///
  /// @brief Construct a new class from a class name
  ///
  /// @param name
  /// @return error IFF name validation failed
  ///
  [[nodiscard]] static Result<Class> From(std::string_view name);

  [[nodiscard]] std::string const& Name() const noexcept;
  [[nodiscard]] std::vector<Field> const& Fields() const noexcept;
  [[nodiscard]] std::vector<Method> const& Methods() const noexcept;
  [[nodiscard]] Point const& Position() const noexcept;

  [[nodiscard]] std::strong_ordering operator<=>(Class const&) const noexcept;

  [[nodiscard]] bool operator==(Class const&) const noexcept;

  ///
  /// @brief Rename a ckass
  ///
  /// @param name the new name
  /// @return error IFF validation of the new class failed
  ///
  [[nodiscard]] Result<void> Rename(std::string_view name);

  ///
  /// @brief Get an iterator to a field
  ///
  /// @param field_name
  /// @return error if the name is malformed or if the field doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Field>::iterator> GetField(std::string_view field_name);

  ///
  /// @brief Get an iterator to a field
  ///
  /// @param field_name
  /// @return error if the name is malformed or if the field doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Field>::const_iterator> GetReadOnlyField(std::string_view field_name) const;

  ///
  /// @brief Get an iterator to a method
  ///
  /// @param method_signature
  /// @return error if the name is malformed or if the method doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Method>::iterator>
  GetMethodFromSignature(MethodSignature const& method_signature);

  ///
  /// @brief Get an iterator to a method
  ///
  /// @param method_signature
  /// @return error if the name is malformed or if the method doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Method>::const_iterator>
  GetReadOnlyMethodFromSignature(MethodSignature const& method_signature) const;

  ///
  /// @brief Get an iterator to a method
  ///
  /// @param method
  /// @return error if the name is malformed or if the method doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Method>::iterator> GetMethod(Method const& method);

  ///
  /// @brief Get an iterator to a method
  ///
  /// @param method
  /// @return error if the name is malformed or if the method doesn't exist
  ///
  [[nodiscard]] Result<std::vector<model::Method>::const_iterator> GetReadOnlyMethod(Method const& method) const;

  ///
  /// @brief Add a field
  ///
  /// @param name
  /// @param type
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> AddField(std::string_view name, std::string_view type);

  ///
  /// @brief Delete a field
  ///
  /// @param field_name
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> DeleteField(std::string_view field_name);

  ///
  /// @brief Rename a field
  ///
  /// @param field_name
  /// @param new_name
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> RenameField(std::string_view field_name, std::string_view new_name);

  ///
  /// @brief Add a method
  ///
  /// @param name
  /// @param return_type
  /// @param parameters
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void>
  AddMethod(std::string_view name, std::string_view return_type, std::vector<Parameter> parameters);

  ///
  /// @brief Delete a method
  ///
  /// @param method_signature
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> DeleteMethod(MethodSignature const& method_signature);

  ///
  /// @brief Rename a method
  ///
  /// @param method_signature
  /// @param new_name
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> RenameMethod(MethodSignature const& method_signature, std::string_view new_name);

  ///
  /// @brief Add a parameter to a method
  ///
  /// @param method_signature
  /// @param parameter_name
  /// @param parameter_type
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> AddParameter(MethodSignature const& method_signature,
                                          std::string_view parameter_name,
                                          std::string_view parameter_type);

  ///
  /// @brief Delete a parameter from a method
  ///
  /// @param method_signature
  /// @param parameter_name
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> DeleteParameter(MethodSignature const& method_signature, std::string_view parameter_name);

  ///
  /// @brief Delete all parameters from a method
  ///
  /// @param method_signature
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> DeleteParameters(MethodSignature const& method_signature);

  ///
  /// @brief Change all parameters of a method
  ///
  /// @param method_signature
  /// @param parameters
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> ChangeParameters(MethodSignature const& method_signature,
                                              std::vector<Parameter> parameters);

  ///
  /// @brief Change the parameter type of a method
  ///
  /// @param method_signature
  /// @param parameter_name
  /// @param new_type
  /// @return error IFF failed
  ///
  [[nodiscard]] Result<void> ChangeParameterType(MethodSignature const& method_signature,
                                                 std::string_view parameter_name,
                                                 std::string_view new_type);

  ///
  /// @brief Move a class to a new location
  ///
  /// @param new_x
  /// @param new_y
  ///
  void Move(int new_x, int new_y);
};

} // namespace model

template <> struct std::formatter<model::Class> {
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    return ctx.begin();
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::Class const& obj, FormatContext& ctx) const {
    constexpr static std::size_t MinWidth{10};
    std::string const name = std::format("{}", obj.Name());
    auto const fields = std::ranges::to<std::vector>(
        std::views::transform(obj.Fields(), [](model::Field const& f) { return std::format("{: }", f); }));
    auto const methods = std::ranges::to<std::vector>(
        std::views::transform(obj.Methods(), [](model::Method const& m) { return std::format("{: }", m); }));
    constexpr auto Max = [](auto x, auto y) { return x > y ? x : y; };
    auto max =
        std::ranges::fold_left(std::views::transform(fields, &std::string::size), Max(name.size(), MinWidth), Max);
    max = std::ranges::fold_left(std::views::transform(methods, &std::string::size), max, Max);
    ctx.advance_to(std::format_to(ctx.out(), "┌─{2:─<{0}}─┐\n│ {1: ^{0}} │\n├─{2:─<{0}}─┤\n", max, name, ""));
    for (auto const& f : fields) {
      ctx.advance_to(std::format_to(ctx.out(), "│ {1: <{0}} │\n", max, f));
    }
    ctx.advance_to(std::format_to(ctx.out(), "├─{1:─<{0}}─┤\n", max, ""));
    for (auto const& m : methods) {
      ctx.advance_to(std::format_to(ctx.out(), "│ {1: <{0}} │\n", max, m));
    }
    ctx.advance_to(std::format_to(ctx.out(), "└─{1:─<{0}}─┘", max, ""));
    return ctx.out();
  }
};
