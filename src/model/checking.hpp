#pragma once

#include "utils/utils.hpp"

#include <algorithm>
#include <concepts>
#include <format>
#include <functional>
#include <ranges>

///
/// @brief Invoke a check function and erase the success value type. On error, display a more meaningful error message.
///
/// @tparam Fn the function to invoke
/// @param entity the string to check
/// @param tag the tag to print in case of failure
/// @return error if the check function fails
///
template <auto Fn> [[nodiscard]] static Result<void> Check(std::string_view entity, std::string_view tag) {
  return std::invoke(Fn, entity, 0).transform([](auto&&...) {}).transform_error([&](std::string&& msg) {
    return std::format("Invalid {}: '{}'. Reason: {}", tag, entity, msg);
  });
}

///
/// @brief Invoke a check function on an entire range and erase the success value type. On error, display a more meaningful error message.
///
/// @tparam Fn the function to invoke
/// @param ranges the strings to check
/// @param tag the tag to print in case of failure
/// @return error if the check function fails
///
template <auto Fn, std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, std::string_view>
[[nodiscard]] static Result<void> CheckAll(Range&& range, std::string_view tag) {
  for (auto const& entity : range) {
    if (auto res = std::invoke(Fn, entity, 0).transform([](auto&&...) {}).transform_error([&](std::string&& msg) {
          return std::format("Invalid {}: '{}'. Reason: {}", tag, entity, msg);
        });
        not res) {
      return res;
    }
  }
  return {};
}

///
/// @brief Ensure a passed container holds only unique values
///
/// @tparam Container
/// @param c the container
/// @param tag the additional context to display in case of failure
/// @return error if duplicates exist
///
template <typename Container> [[nodiscard]] Result<void> Unique(Container&& c, std::string_view tag) {
  auto copy{std::forward<Container>(c)};
  std::ranges::sort(copy);
  if (not std::ranges::unique(copy).empty()) {
    return std::unexpected{std::format("Duplicate {} exist", tag)};
  } else {
    return {};
  }
}
