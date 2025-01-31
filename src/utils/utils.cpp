#include "utils.hpp"

#include <doctest/doctest.h>

#include <charconv>
#include <format>
#include <ranges>

Result<std::size_t> ValidIdentifier(std::string_view t, std::size_t start) noexcept {
  if (start == t.size()) {
    return std::unexpected{std::format("expected identifier but was empty")};
  }
  if (not Alpha(t[start])) {
    return std::unexpected{std::format("expected identifier saw non-alphabetic '{}' at index {}", t[start], start)};
  }
  ++start;
  while (start < t.size() and AlNum(t[start])) {
    ++start;
  }
  return start;
}

/// TList -> [ T [comma T]* ]

/// T -> A [ lsep TList rsep ]

Result<std::size_t> ValidType(std::string_view t, std::size_t start) {
  auto id = ValidIdentifier(t, start);
  if (not id) {
    return id;
  }
  if (*id == t.size()) {
    return id;
  }
  start = *id;
  static std::vector<char> stack;
  switch (t[start]) {
  case '[':
    stack.push_back(']');
    break;
  case '(':
    stack.push_back(')');
    break;
  case '<':
    stack.push_back('>');
    break;
  case '*':
    // consume any number of *s
    ++start;
    while (start < t.size() and t[start] == '*') {
      ++start;
    }
    return start;
  default:
    return start;
  }
  if (++start == t.size()) {
    stack.pop_back();
    return std::unexpected{"Expected more after type specifier"};
  }
  bool first{true};
  // while we don't see a matching bracket
  while (t[start] != stack.back()) {
    if (first) {
      first = false;
    } else {
      if (t[start] != ',') {
        return std::unexpected{std::format("Expected ',' but got '{}' at index {}", t[start], start)};
      } else {
        ++start;
      }
    }
    if (auto type = ValidType(t, start); not type) {
      stack.pop_back();
      return type;
    } else {
      start = *type;
    }
    if (start == t.size()) {
      stack.pop_back();
      return std::unexpected{"Unexpected end to type list"};
    }
  }
  stack.pop_back();
  // consume any number of *s
  ++start;
  while (start < t.size() and t[start] == '*') {
    ++start;
  }
  return start;
}

std::vector<std::string_view> Split(std::string_view str) {
  return str | std::views::split(' ') | std::views::filter([](auto&& s) { return not s.empty(); }) |
         std::views::transform([](auto&& s) { return std::string_view{s.begin(), s.end()}; }) |
         std::ranges::to<std::vector>();
}

Result<int> IntFromString(std::string_view s) {
  int value;
  auto [ptr, ec] = std::from_chars(s.begin(), s.end(), value);
  if (ptr == s.end() and ec == std::errc{}) {
    return value;
  } else {
    return std::unexpected{std::format("Couldn't parse number from string: {}", s)};
  }
}

DOCTEST_TEST_SUITE("utils") {
  DOCTEST_TEST_CASE("utils::Alpha") {
    for (auto c = static_cast<char>(-1); c < 127;) {
      ++c;
      CHECK_EQ((('A' <= c and c <= 'Z') or ('a' <= c and c <= 'z') or (c == '_')), Alpha(c));
    }
  }
  DOCTEST_TEST_CASE("utils::Digit") {
    for (auto c = static_cast<char>(-1); c < 127;) {
      ++c;
      CHECK_EQ(('0' <= c and c <= '9'), Digit(c));
    }
  }
  DOCTEST_TEST_CASE("utils::AlNum") {
    for (auto c = static_cast<char>(-1); c < 127;) {
      ++c;
      CHECK_EQ((('A' <= c and c <= 'Z') or ('a' <= c and c <= 'z') or (c == '_') or ('0' <= c and c <= '9')), AlNum(c));
    }
  }
  DOCTEST_TEST_CASE("utils::ValidIdentifier") {
    CHECK_EQ(ValidIdentifier("Alpha ").value_or(0), 5);
    CHECK_EQ(ValidIdentifier("_Test").value_or(0), 5);
    CHECK_EQ(ValidIdentifier("test3").value_or(0), 5);
    CHECK_FALSE(ValidIdentifier("").has_value());
    CHECK_FALSE(ValidIdentifier("test", 4).has_value());
    CHECK_FALSE(ValidIdentifier("1test").has_value());
    CHECK_FALSE(ValidIdentifier("<test>").has_value());
    CHECK_FALSE(ValidIdentifier("(test)").has_value());
  }
  DOCTEST_TEST_CASE("utils::ValidType") {
    CHECK_EQ(ValidType("Alpha ").value_or(0), 5);
    CHECK_EQ(ValidType("_Test").value_or(0), 5);
    CHECK_EQ(ValidType("test3").value_or(0), 5);
    CHECK_EQ(ValidType("Alph* ").value_or(0), 5);
    CHECK_EQ(ValidType("_Tes*").value_or(0), 5);
    CHECK_EQ(ValidType("tes**").value_or(0), 5);
    CHECK_FALSE(ValidType("Alpha<").has_value());
    CHECK_FALSE(ValidType("Alpha(").has_value());
    CHECK_FALSE(ValidType("Alpha[").has_value());
    CHECK_EQ(ValidType("Alpha<>").value_or(0), 7);
    CHECK_EQ(ValidType("Alpha[]").value_or(0), 7);
    CHECK_EQ(ValidType("Alpha()").value_or(0), 7);
    CHECK_EQ(ValidType("A<int>").value_or(0), 6);
    CHECK_EQ(ValidType("A[int]").value_or(0), 6);
    CHECK_EQ(ValidType("A[int]").value_or(0), 6);
    CHECK_FALSE(ValidType("A<int"));
    CHECK_FALSE(ValidType("A[int"));
    CHECK_FALSE(ValidType("A[int"));
    CHECK_EQ(ValidType("A<int,int>").value_or(0), 10);
    CHECK_EQ(ValidType("A[int,int]").value_or(0), 10);
    CHECK_EQ(ValidType("A[int,int]").value_or(0), 10);
    CHECK_EQ(ValidType("A<int*,int**>*").value_or(0), 14);
    CHECK_EQ(ValidType("A[int*,int**]*").value_or(0), 14);
    CHECK_EQ(ValidType("A[int*,int**]*").value_or(0), 14);
    CHECK_FALSE(ValidType("A<int,int"));
    CHECK_FALSE(ValidType("A(int,int"));
    CHECK_FALSE(ValidType("A[int,int"));
    CHECK_FALSE(ValidType("A[int,int,"));
    CHECK_FALSE(ValidType("A[int^int"));
    CHECK_EQ(ValidType("A<B[int],C(int)>").value_or(0), 16);
    CHECK_EQ(ValidType("A<B(int),C[int]>").value_or(0), 16);
    CHECK_EQ(ValidType("A[B<int>,C(int)]").value_or(0), 16);
    CHECK_EQ(ValidType("A[B(int),C<int>]").value_or(0), 16);
    CHECK_EQ(ValidType("A[B<int>,C(int)]").value_or(0), 16);
    CHECK_EQ(ValidType("A[B(int),C<int>]").value_or(0), 16);
    CHECK_FALSE(ValidType("A<B[int],C(int)"));
    CHECK_FALSE(ValidType("A<B(int),C[int]"));
    CHECK_FALSE(ValidType("A[B<int>,C(int)"));
    CHECK_FALSE(ValidType("A[B(int),C<int>"));
    CHECK_FALSE(ValidType("A[B<int>,C(int)"));
    CHECK_FALSE(ValidType("A[B(int),C<int>"));
  }
  DOCTEST_TEST_CASE("utils::Split") {
    CHECK_EQ(Split("hello"), std::vector<std::string_view>{"hello"});
    CHECK_EQ(Split("hello world"), std::vector<std::string_view>{"hello", "world"});
    CHECK_EQ(Split("hello     world"), std::vector<std::string_view>{"hello", "world"});
    CHECK_EQ(Split("     hello     world"), std::vector<std::string_view>{"hello", "world"});
    CHECK_EQ(Split("     hello     world     "), std::vector<std::string_view>{"hello", "world"});
  }
  DOCTEST_TEST_CASE("utils::IntFromString") {
    CHECK_EQ(IntFromString("0").value_or(-1), 0);
    CHECK_EQ(IntFromString("120").value_or(0), 120);
    CHECK_EQ(IntFromString("9384").value_or(0), 9384);
    CHECK_EQ(IntFromString("-147").value_or(0), -147);
    CHECK_FALSE(IntFromString("123a").has_value());
    CHECK_FALSE(IntFromString("123.0").has_value());
    CHECK_FALSE(IntFromString("a123").has_value());
    CHECK_FALSE(IntFromString("123 ").has_value());
    CHECK_FALSE(IntFromString(" 123").has_value());
  }
}
