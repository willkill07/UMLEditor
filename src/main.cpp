#include "cli/readline_view.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <print>
#include <ranges>
#include <string_view>
#include <vector>

#define Evaluate(x) #x
#define PreprocessorDefined(x) std::string_view(#x) != std::string_view(Evaluate(x))

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv) {
  auto const args = std::ranges::to<std::vector<std::string_view>>(std::ranges::subrange{argv, argv + argc});
  if ((args.size() == 1) or (args[1] == "--cli" and args.size() == 2)) {
    return cli::CLI();
  } else if constexpr (PreprocessorDefined(DOCTEST_CONFIG_DISABLE)) {
    std::println("Usage: {} [--cli]", argv[0]);
  } else if (args[1] == "--tests") {
    return doctest::Context(argc, argv).run();
  } else {
    std::println("Usage: {} [--cli|--tests] [test_args...]", argv[0]);
  }
}
