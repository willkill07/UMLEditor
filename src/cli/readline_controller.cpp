#include "readline_controller.hpp"

#include "commands/commands.hpp"
#include "commands/completers.hpp"
#include "model/diagram.hpp"
#include "utils/io_context.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>

#include <editline/readline.h>

#include <cstring>

#include <algorithm>
#include <print>
#include <ranges>
#include <utility>

namespace cli {

struct CommandTree {
  std::string name;
  std::vector<CommandTree> subcommands;
  constexpr CommandTree() noexcept = delete;
  constexpr explicit CommandTree(std::string n) noexcept : CommandTree(std::move(n), {}) {
  }
  constexpr CommandTree(std::string n, std::vector<CommandTree> s) noexcept
      : name(std::move(n)), subcommands(std::move(s)) {
  }
};

static CommandTree& GetTree() {
  static std::optional<CommandTree> root{std::nullopt};
  if (not root) {
    CommandTree r{""};
    for (std::string_view command : commands::CommandStrings) {
      CommandTree* current = std::addressof(r);
      for (auto part : std::views::split(command, ' ')) {
        std::string_view name{part.begin(), part.end()};
        if (auto const found = std::ranges::find(current->subcommands, name, &CommandTree::name);
            found == current->subcommands.end()) {
          current->subcommands.emplace_back(std::string{name});
          current = std::addressof(current->subcommands.back());
        } else {
          current = std::addressof(*found);
        }
      }
    }
    root = std::move(r);
  }
  return root.value();
}

static CommandTree* curr_command = nullptr;
static commands::Completer completer = std::monostate{};

static std::vector<std::string> candidates{};
static std::size_t candidate_index{0};

static char* AutocompleteCommon(std::string_view word) {
  for (auto const& c : candidates | std::views::drop(candidate_index)) {
    ++candidate_index;
    if (c.starts_with(word)) {
      return strdup(c.c_str());
    }
  }
  return nullptr;
}

static char* BuiltinAutocomplete(char const* text, int state) {
  if (state == 0) {
    candidate_index = 0;
    candidates.clear();
    if (curr_command != nullptr) {
      std::ranges::copy(curr_command->subcommands | std::views::filter([](CommandTree const& c) {
                          return not c.name.starts_with('[');
                        }) | std::views::transform([](CommandTree const& c) { return std::string{c.name}; }),
                        std::back_inserter(candidates));
    }
  }
  return AutocompleteCommon(text);
}

static char* DynamicAutocomplete(char const* text, int state) {
  if (state == 0) {
    candidate_index = 0;
    candidates.clear();
    std::visit(
        []<typename T>(T&& c) {
          if constexpr (not std::is_same_v<std::monostate, std::decay_t<T>>) {
            candidates = c.Candidates();
          }
        },
        completer);
  }
  return AutocompleteCommon(text);
}

char** RegisterCompletions(char const* text, int start, int end) {
  rl_attempted_completion_over = 1;
  std::string_view line{rl_line_buffer};
  std::string_view substring{line.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start))};
  if (std::cmp_less(end, line.size())) {
    return nullptr;
  }

  auto completed = Split(line);
  // if our current token is empty and we already have a command, then imbue an empty string
  if (substring.empty() and not completed.empty()) {
    completed.emplace_back("");
  }

  // always start at the root
  curr_command = std::addressof(GetTree());
  completer = std::monostate{};
  for (auto [index, word] : std::views::zip(std::views::iota(0), completed)) {
    if (auto i = std::ranges::find_if(
            curr_command->subcommands,
            [&](std::string_view cmd_name) { return cmd_name == word or cmd_name.starts_with('['); },
            &CommandTree::name);
        i == curr_command->subcommands.end()) {
      // no match -> reset
      if (curr_command->subcommands.empty()) {
        completer = std::monostate{};
        curr_command = nullptr;
      }
      break;
    } else {
      std::string_view token = i->name;
      // we have a match -- act upon it
      if (token.starts_with('[')) {
        if (token == "[class_name]") {
          completer = commands::ClassCompleter{.diagram = model::Diagram::GetInstance(), .name = word};
        } else if (token == "[field_name]") {
          auto prev = std::get<commands::ClassCompleter>(completer);
          completer = commands::FieldCompleter{.iter = prev.Get(), .name = word};
        } else if (token == "[method_signature]") {
          auto prev = std::get<commands::ClassCompleter>(completer);
          completer = commands::MethodCompleter{.iter = prev.Get(), .signature = word};
        } else if (token == "[param_name]") {
          auto prev = std::get<commands::MethodCompleter>(completer);
          completer = commands::ParameterCompleter{.iter = prev.Get(), .name = word};
        } else if (token == "[class_source]") {
          completer = commands::RelationshipSourceCompleter{.diagram = model::Diagram::GetInstance(), .source = word};
        } else if (token == "[class_destination]") {
          auto prev = std::get<commands::RelationshipSourceCompleter>(completer);
          completer =
              commands::RelationshipDestinationCompleter{.diagram = prev.diagram, .source = prev.source, .dest = word};
        } else if (token == "[relationship_type]") {
          completer = commands::RelationshipTypeCompleter{};
        } else if (token == "[filename]") {
          completer = std::monostate{};
          // fall back to file autocomplete
          rl_attempted_completion_over = 0;
        } else {
          completer = std::monostate{};
        }
      }
      // only update the command if we have more to parse
      if (std::cmp_not_equal(index + 1, completed.size())) {
        curr_command = std::addressof(*i);
      }
    }
  }
  if (completer.index() == 0) {
    // if we have no command (or if load/save) then return no autocomplete options
    if (curr_command == nullptr or curr_command->name == "load" or curr_command->name == "save") {
      return nullptr;
    }
    return rl_completion_matches(text, BuiltinAutocomplete);
  } else {
    return rl_completion_matches(text, DynamicAutocomplete);
  }
}

ReadlineInterface::ReadlineInterface(std::string_view prompt) : prompt_{prompt} {
  static std::array sep{' ', '\t', '\n', '\0'};
  rl_basic_word_break_characters = sep.data();
  rl_attempted_completion_function = RegisterCompletions;
}

[[nodiscard]] bool ReadlineInterface::ReadCommand() {
  buffer_.reset(readline(prompt_.c_str()));
  return buffer_ != nullptr;
}

[[nodiscard]] std::string_view ReadlineInterface::GetCommand() const {
  return buffer_.get();
}

[[nodiscard]] std::vector<std::string_view> ReadlineInterface::GetTokenizedCommand() const {
  return Split(GetCommand());
}

void ReadlineInterface::DisplayMessage(std::string_view message) const {
  std::println(stderr, "{}", message);
}

void ReadlineInterface::AddCommandToHistory() const {
  add_history(buffer_.get());
}

} // namespace cli

///
/// \brief Adaptor to trick readline-like API to replace its buffer with a provided line and
///        cursor position and retrieve the list of possible completions.
///
/// \param line the text to set the readline's buffer to
/// \param end the cursor position
/// \return list of all completions
///
ENABLE_IF_TEST(static std::vector<std::string> GetCompletionsForLine(std::string const& line,
                                                                     std::optional<int> end = std::nullopt) {
  int start = static_cast<int>(line.find_last_of(' ')) + 1;
  char* old = std::exchange(rl_line_buffer, strdup(line.c_str()));
  rl_end = rl_point = static_cast<int>(line.size());
  char** response = cli::RegisterCompletions(rl_line_buffer + start, start, end.value_or(rl_end));
  free(std::exchange(rl_line_buffer, old));
  std::vector<std::string> completions;
  for (int i{0}; response != nullptr and response[i] != nullptr; ++i) {
    if (i != 0) {
      completions.emplace_back(response[i]);
    }
    free(response[i]);
  }
  if (response) {
    free(reinterpret_cast<void*>(response));
  }
  return completions;
})

DOCTEST_TEST_SUITE("cli") {
  DOCTEST_TEST_CASE("cli::RegisterCompletions") {
    std::vector<std::string> list;

    ENABLE_IF_TEST(list = GetCompletionsForLine(""));
    CHECK(std::ranges::contains(list, "class"));
    CHECK(std::ranges::contains(list, "exit"));
    CHECK(std::ranges::contains(list, "field"));
    CHECK(std::ranges::contains(list, "help"));
    CHECK(std::ranges::contains(list, "list"));
    CHECK(std::ranges::contains(list, "load"));
    CHECK(std::ranges::contains(list, "method"));
    CHECK(std::ranges::contains(list, "parameter"));
    CHECK(std::ranges::contains(list, "parameters"));
    CHECK(std::ranges::contains(list, "redo"));
    CHECK(std::ranges::contains(list, "relationship"));
    CHECK(std::ranges::contains(list, "save"));
    CHECK(std::ranges::contains(list, "undo"));
    CHECK_EQ(list.size(), 13);

    ENABLE_IF_TEST(list = GetCompletionsForLine("p"));
    CHECK(std::ranges::contains(list, "parameter"));
    CHECK(std::ranges::contains(list, "parameters"));
    CHECK_EQ(list.size(), 2);

    ENABLE_IF_TEST(list = GetCompletionsForLine("class "));
    CHECK(std::ranges::contains(list, "add"));
    CHECK(std::ranges::contains(list, "remove"));
    CHECK(std::ranges::contains(list, "rename"));
    CHECK_EQ(list.size(), 3);

    auto& d = model::Diagram::GetInstance();
    REQUIRE(d.AddClass("alpha"));
    REQUIRE(d.AddClass("artist"));
    REQUIRE(d.AddClass("beta"));

    ENABLE_IF_TEST(list = GetCompletionsForLine("class remove "));
    CHECK(std::ranges::contains(list, "alpha"));
    CHECK(std::ranges::contains(list, "artist"));
    CHECK(std::ranges::contains(list, "beta"));
    CHECK_EQ(list.size(), 3);

    ENABLE_IF_TEST(list = GetCompletionsForLine("class remove a"));
    CHECK(std::ranges::contains(list, "alpha"));
    CHECK(std::ranges::contains(list, "artist"));
    CHECK_EQ(list.size(), 2);

    [[maybe_unused]] auto alpha = d.GetClass("alpha").value();

    REQUIRE(alpha->AddField("x", "int"));
    REQUIRE(alpha->AddField("y", "int"));
    ENABLE_IF_TEST(list = GetCompletionsForLine("field remove alpha "));
    CHECK(std::ranges::contains(list, "x"));
    CHECK(std::ranges::contains(list, "y"));
    CHECK_EQ(list.size(), 2);

    REQUIRE(alpha->AddMethod("fun", "void", *model::Parameter::MultipleFromString("enable:bool,flag:bool")));
    REQUIRE(alpha->AddMethod("fun", "int", {}));
    ENABLE_IF_TEST(list = GetCompletionsForLine("method remove alpha "));
    CHECK(std::ranges::contains(list, "fun()"));
    CHECK(std::ranges::contains(list, "fun(bool,bool)"));
    CHECK_EQ(list.size(), 2);

    ENABLE_IF_TEST(list = GetCompletionsForLine("parameter remove alpha fun(bool,bool) "));
    CHECK(std::ranges::contains(list, "enable"));
    CHECK(std::ranges::contains(list, "flag"));
    CHECK_EQ(list.size(), 2);

    ENABLE_IF_TEST(list = GetCompletionsForLine("relationship add alpha beta "));
    CHECK(std::ranges::contains(list, "Aggregation"));
    CHECK(std::ranges::contains(list, "Composition"));
    CHECK(std::ranges::contains(list, "Inheritance"));
    CHECK(std::ranges::contains(list, "Realization"));
    CHECK_EQ(list.size(), 4);

    REQUIRE(d.AddRelationship("alpha", "alpha", model::RelationshipType::Composition));
    REQUIRE(d.AddRelationship("alpha", "beta", model::RelationshipType::Inheritance));
    ENABLE_IF_TEST(list = GetCompletionsForLine("relationship remove "));
    CHECK(std::ranges::contains(list, "alpha"));
    CHECK_EQ(list.size(), 1);

    ENABLE_IF_TEST(list = GetCompletionsForLine("relationship remove alpha "));
    CHECK(std::ranges::contains(list, "alpha"));
    CHECK(std::ranges::contains(list, "beta"));
    CHECK_EQ(list.size(), 2);

    ENABLE_IF_TEST(list = GetCompletionsForLine("load b"));
    CHECK(list.empty());

    ENABLE_IF_TEST(list = GetCompletionsForLine("save b"));
    CHECK(list.empty());

    ENABLE_IF_TEST(list = GetCompletionsForLine("invalid command"));
    CHECK(list.empty());

    ENABLE_IF_TEST(list = GetCompletionsForLine("class add "));
    CHECK(list.empty());

    ENABLE_IF_TEST(list = GetCompletionsForLine("class", 2));
    CHECK(list.empty());

    ENABLE_IF_TEST(list = GetCompletionsForLine("list all everything"));
    CHECK(list.empty());

    REQUIRE(d.DeleteClass("alpha"));
    REQUIRE(d.DeleteClass("artist"));
    REQUIRE(d.DeleteClass("beta"));
    REQUIRE(d.GetClasses().empty());
    REQUIRE(d.GetRelationships().empty());
  }
  DOCTEST_TEST_CASE("cli::ReadlineInterface") {
    ENABLE_IF_TEST(bool result; std::string command; std::vector<std::string> tokens; std::string err; std::string out;
                   {
                     IOContext ctx;
                     cli::ReadlineInterface cli{"UML> "};
                     ctx.StdIn("class add x\n");
                     result = cli.ReadCommand();
                     cli.AddCommandToHistory();
                     command = std::ranges::to<std::string>(cli.GetCommand());
                     tokens = std::ranges::to<std::vector<std::string>>(cli.GetTokenizedCommand());
                     cli.DisplayMessage("test");
                     err = ctx.StdErr();
                     out = ctx.StdOut();
                   })
    CHECK(result);
    CHECK_EQ(command, "class add x");
    CHECK_EQ(tokens, std::vector<std::string>{"class", "add", "x"});
    CHECK_EQ(err, "test\n");
    CHECK_EQ(out, "");
  }
}
