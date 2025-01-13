#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

template <typename T> using Result = std::expected<T, std::string>;

template <typename E> using Error = std::unexpected<E>;

///
/// \brief Returns whether a passed character is within a defined inclusive range
///
template <char Lo, char Hi> static inline constexpr bool InRange(char c) noexcept {
  return Lo <= c and c <= Hi;
}

///
/// @brief Returns whether a passed character is alphabetic (or _)
///
static inline constexpr bool Alpha(char c) noexcept {
  return InRange<'a', 'z'>(c) or InRange<'A', 'Z'>(c) or c == '_';
}

///
/// @brief Returns whether a passed character is a digit
///
static inline constexpr bool Digit(char c) noexcept {
  return InRange<'0', '9'>(c);
}

///
/// @brief Returns whether a passed character is alphabetic (or _) or a digit
///
static inline constexpr bool AlNum(char c) noexcept {
  return Alpha(c) or Digit(c);
}

///
/// @brief Determine if the specified offset of a string_view starts with a valid identifier
///
/// @param t the token
/// @param start the offset (defaults to 0)
/// @return Error if it is not a valid identifier or the end offset of the valid identifier
///
[[nodiscard]] Result<std::size_t> ValidIdentifier(std::string_view t, std::size_t start = 0) noexcept;

///
/// @brief Determine if the specified offset of a string_view starts with a valid type
///
/// @param t the token
/// @param start the offset (defaults to 0)
/// @return Error if it is not a valid identifier or the end offset of the valid type
///
[[nodiscard]] Result<std::size_t> ValidType(std::string_view t, std::size_t start = 0);

///
/// @brief Split a string_view by spaces
///
/// @param str the string_view to split
/// @return a vector of string_views
///
[[nodiscard]] std::vector<std::string_view> Split(std::string_view str);

///
/// @brief Parse an int from a string
///
/// @param s the string
/// @return Error if parsing could not be performed else the held integer
///
[[nodiscard]] Result<int> IntFromString(std::string_view s);
