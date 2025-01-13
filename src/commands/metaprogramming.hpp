#pragma once

#include "model/method.hpp"
#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "model/relationship_type.hpp"

#include <algorithm>
#include <ranges>
#include <tuple>
#include <type_traits>

namespace commands {

///
/// @brief Get the command argument count
///
/// @param command the command to check
/// @return the number of words in the compile-time string view
///
[[nodiscard]] static consteval auto CommandLength(std::string_view command) {
  return 1 + std::ranges::count(command, ' ');
}

///
/// @brief Get the command parameter count
///
/// Parameters start with [ so just count the number
///
/// @param command the command to check
/// @return the number of words in the compile-time string view
///
[[nodiscard]] static consteval auto CommandParamCount(std::string_view command) {
  return std::ranges::count(command, '[');
}

///
/// @brief Get the parameter at the specified index
///
/// @param command the command to check
/// @param index the zero-based index of the parameter
/// @return the extracted command
///
[[nodiscard]] static consteval std::string_view GetParam(std::string_view command, std::size_t index) {
  std::size_t const start{
      std::ranges::fold_left(std::views::iota(0ZU, index), command.find('['), [&](std::size_t idx, std::size_t) {
        return command.find('[', idx + 1);
      })};
  std::size_t end{command.find(']', start + 1)};
  return command.substr(start, end - start + 1);
}

///
/// @brief Parse the runtime argument based on the compile-time command and parameter index
///
/// @tparam Command the command to check
/// @tparam Index the zero-based index of the parameter to parse
/// @param arg the runtime command argument
/// @return the parsed runtime value
///
template <typename Command, std::size_t Index> static constexpr auto ParseParam(std::string_view arg) {
  if constexpr (constexpr std::string_view param = GetParam(Command::CommandName, Index); param == "[param_list]") {
    // param list is a vector<Parameter>
    return model::Parameter::MultipleFromString(arg);
  } else if constexpr (param == "[method_signature]") {
    // method_signature is MethodSignature
    return model::MethodSignature::FromString(arg);
  } else if constexpr (param == "[method_definition]") {
    // method_definition is Method
    return model::Method::FromString(arg);
  } else if constexpr (param == "[relationship_type]") {
    // relationship_type is RelationshipType
    return model::RelationshipTypeFromString(arg);
  } else if constexpr (param == "[name]" or              //
                       param == "[type]" or              //
                       param == "[class_name]" or        //
                       param == "[class_source]" or      //
                       param == "[class_destination]" or //
                       param == "[param_name]" or        //
                       param == "[field_name]" or        //
                       param == "[filename]") {
    // everything else is "identity" (a.k.a. string)
    return Result<std::string>{arg};
  } else if constexpr (param == "[int]") {
    return IntFromString(arg);
  } else {
    static_assert(false, "unsupported parameter");
  }
}

namespace detail {

template <typename Command, std::size_t I> using ParseFn = decltype(&ParseParam<Command, I>);

template <typename Command, std::size_t I>
using TypeFor =
    std::remove_reference_t<typename std::invoke_result_t<ParseFn<Command, I>, std::string_view>::value_type>;

template <typename, typename> struct TupleFor;

template <typename Command, std::size_t... Is>
struct TupleFor<Command, std::index_sequence<Is...>> : std::type_identity<std::tuple<TypeFor<Command, Is>...>> {};

} // namespace detail

template <typename Command>
using TupleFor = detail::TupleFor<Command, std::make_index_sequence<CommandParamCount(Command::CommandName)>>::type;

} // namespace commands
