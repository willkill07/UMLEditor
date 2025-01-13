#pragma once

#ifdef DOCTEST_CONFIG_DISABLE
#define ENABLE_IF_TEST(...)
#else
#define ENABLE_IF_TEST(...) __VA_ARGS__

#include <string>
#include <string_view>

struct IOContext {
  IOContext();

  ~IOContext();

  [[nodiscard]] std::string StdOut() const;

  [[nodiscard]] std::string StdErr() const;

  void StdIn(std::string_view data) const;

private:
  // NOLINTBEGIN(modernize-avoid-c-arrays)
  int mapping_[3];
  int saved_[3];
  int pipes_[3][2];
  // NOLINTEND(modernize-avoid-c-arrays)
};

#endif
