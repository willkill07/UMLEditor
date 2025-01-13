#include "base_commands.hpp"

#include "commands/commands.hpp"
#include "model/diagram.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>

namespace commands {

///
/// @brief Extract all runtime command parameter values
///
/// @tparam Command the command to check
/// @param args the runtime command arguments
/// @return an array only containing the runtime parameter values
///
template <typename Command, auto Count = CommandParamCount(Command::CommandName)>
[[nodiscard]] static Result<std::array<std::string_view, Count>> GetParams(std::span<std::string_view> args) {
  if (constexpr auto Size = CommandLength(Command::CommandName); args.size() == Size) {
    std::array<std::string_view, Count> arr;
    for (auto i = arr.begin(); auto [cmd, arg] : std::views::zip(std::views::split(Command::CommandName, ' '), args)) {
      if (cmd[0] == '[') {
        *i++ = arg;
      }
    }
    return arr;
  } else {
    return std::unexpected{std::format("Invalid number of arguments: got {} but expected {}", args.size(), Size)};
  }
}

///
/// @brief Make a command based on the runtime span of arguments
///
/// @tparam Command the command
/// @param entered the runtime span of arguments
/// @return an error if the Command could not be constructed; otherwise, the constructed command
///
template <typename Command> [[nodiscard]] static auto MakeCommand(std::span<std::string_view> entered) {
  return GetParams<Command>(entered).and_then([](auto&& args) -> Result<std::shared_ptr<Command>> {
    return std::apply(
        [](auto&&... p) -> Result<std::shared_ptr<Command>> {
          Result<void> r{};
          // ensure that all arguments parsed are valid
          std::ignore = (true && ... && (r = p.transform([](auto&&...) {})));
          return r.transform([&] { return std::make_shared<Command>(std::tuple{std::move(*p)...}); })
              .transform_error([&](std::string&& msg) {
                return std::format("Error: {}. Usage: '{}'", std::move(msg), Command::CommandName);
              });
        },
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
          return std::tuple(ParseParam<Command, Is>(std::move(args[Is]))...);
        }(std::make_index_sequence<CommandParamCount(Command::CommandName)>{}));
  });
}

Command::~Command() noexcept = default;

Result<std::shared_ptr<Command>> Command::From(std::span<std::string_view> tokens) {
  if (tokens.empty()) {
    return std::unexpected{"Empty command"};
  }
  static constexpr std::size_t NumCommands{std::tuple_size_v<commands::AllCommands>};
  // build this once so we don't need to do it again
  static auto const SPLIT = std::ranges::to<std::vector>(std::views::transform(commands::CommandStrings, Split));
  auto seq = std::views::iota(0ZU, NumCommands);
  std::vector cmds(seq.begin(), seq.end());
  for (std::size_t index{0}; not cmds.empty() and index < tokens.size(); ++index) {
    std::string_view token{tokens[index]};
    // filter based on the next token entered
    std::erase_if(cmds, [index, token](std::size_t i) { return index >= SPLIT[i].size() or SPLIT[i][index] != token; });
    if (cmds.size() == 1) {
      // we have an exact command, so make the correct command
      return []<std::size_t... I>(std::size_t idx, std::span<std::string_view> toks, std::index_sequence<I...>) {
        std::optional<Result<std::shared_ptr<Command>>> c{std::nullopt};
        // runtime index -> compile-time index
        (... || [&]<std::size_t J>() {
          if (J == idx) {
            c.emplace(MakeCommand<std::tuple_element_t<I, AllCommands>>(toks));
          }
          return c;
        }.template operator()<I>());
        // optional will definitely hold a value
        return c.value();
      }(cmds.front(), tokens, std::make_index_sequence<NumCommands>{});
    }
  }
  if (cmds.empty()) {
    return std::unexpected{"Invalid command. View a list of commands with 'help'"};
  } else {
    std::string output{"Command requires subcommand:"};
    for (std::size_t const i : cmds) {
      output.append("\n  ");
      output.append(commands::CommandStrings[i]);
    }
    return std::unexpected{output};
  }
}

Result<void> Command::Undo(model::Diagram& diagram) const {
  if (not Trackable()) {
    return {};
  } else if (prior_state_) {
    diagram = *prior_state_;
    return {};
  } else {
    return std::unexpected{"No prior state to restore"};
  }
}

bool Command::Trackable() const noexcept {
  return true;
}

Result<void> Command::Commit(model::Diagram& diagram) {
  prior_state_ = std::make_unique<model::Diagram>(diagram);
  return Execute(diagram);
}

UntrackableCommand::~UntrackableCommand() noexcept = default;

bool UntrackableCommand::Trackable() const noexcept {
  return false;
}

} // namespace commands

DOCTEST_TEST_SUITE("commands::Command") {
  DOCTEST_TEST_CASE("commands::Command.From") {
    DOCTEST_SUBCASE("commands::Command.From.Builtin") {
      auto cmd = Split("");
      CHECK_FALSE(commands::Command::From(cmd));

      cmd = Split("invalid command");
      CHECK_FALSE(commands::Command::From(cmd));

      cmd = Split("load");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("load file");
      CHECK(commands::Command::From(cmd));

      cmd = Split("save");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("save file");
      CHECK(commands::Command::From(cmd));

      cmd = Split("list");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("list all");
      CHECK(commands::Command::From(cmd));
      cmd = Split("list classes");
      CHECK(commands::Command::From(cmd));
      cmd = Split("list relationships");
      CHECK(commands::Command::From(cmd));
      cmd = Split("list class");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("list class x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("help");
      CHECK(commands::Command::From(cmd));
      cmd = Split("help x");
      CHECK_FALSE(commands::Command::From(cmd));

      cmd = Split("exit");
      CHECK(commands::Command::From(cmd));
      cmd = Split("exit x");
      CHECK_FALSE(commands::Command::From(cmd));

      cmd = Split("undo");
      CHECK(commands::Command::From(cmd));
      cmd = Split("undo x");
      CHECK_FALSE(commands::Command::From(cmd));

      cmd = Split("redo");
      CHECK(commands::Command::From(cmd));
      cmd = Split("redo x");
      CHECK_FALSE(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Class") {
      auto cmd = Split("class");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("class add");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("class add x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("class remove");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("class remove x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("class rename");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("class rename x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("class rename x y");
      CHECK(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Field") {
      auto cmd = Split("field");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field add");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field add x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field add x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field add x x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("field remove");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field remove x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field remove x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("field rename");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field rename x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field rename x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field rename x x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("field retype");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field retype x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field retype x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("field retype x x x");
      CHECK(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Method") {
      auto cmd = Split("method");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method add");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method add x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method add x y");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method add x y()->z");
      CHECK(commands::Command::From(cmd));

      cmd = Split("method remove");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method remove x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method remove x y");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method remove x y()");
      CHECK(commands::Command::From(cmd));

      cmd = Split("method rename");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method rename x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method rename x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method rename x y z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method rename x y() z");
      CHECK(commands::Command::From(cmd));

      cmd = Split("method change-return-type");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method change-return-type x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method change-return-type x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method change-return-type x y z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("method change-return-type x y() z");
      CHECK(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Parameter") {
      auto cmd = Split("parameter");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add x x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add x y z w");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter add x y() z w");
      CHECK(commands::Command::From(cmd));

      cmd = Split("parameter remove");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter remove x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter remove x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter remove x y z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter remove x y() z");
      CHECK(commands::Command::From(cmd));

      cmd = Split("parameter rename");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter rename x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter rename x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter rename x x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter rename x y z w");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter rename x y() z w");
      CHECK(commands::Command::From(cmd));

      cmd = Split("parameter retype");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter retype x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter retype x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter retype x x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter retype x y z w");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameter retype x y() z w");
      CHECK(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Parameter") {
      auto cmd = Split("parameters clear");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters clear x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters clear x y");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters clear x y(int)");
      CHECK(commands::Command::From(cmd));

      cmd = Split("parameters set");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters set x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters set x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters set x y a:int,b:int");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters set x y(int) z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("parameters set x y(int) a:int,b:int");
      CHECK(commands::Command::From(cmd));
    }
    DOCTEST_SUBCASE("commands::Command.From.Relationship") {
      auto cmd = Split("relationship");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship add");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship add x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship add x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship add x x z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship add x x Aggregation");
      CHECK(commands::Command::From(cmd));

      cmd = Split("relationship remove");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship remove x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship remove x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("relationship change");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change source");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change source x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change source x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change source x x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("relationship change destination");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change destination x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change destination x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change destination x x x");
      CHECK(commands::Command::From(cmd));

      cmd = Split("relationship change type");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change type x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change type x x");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change type x x z");
      CHECK_FALSE(commands::Command::From(cmd));
      cmd = Split("relationship change type x x Composition");
      CHECK(commands::Command::From(cmd));
    }
  }
  DOCTEST_TEST_CASE("commands::Command::Commit") {
    auto cmd = Split("class add x");
    auto add = commands::Command::From(cmd);
    REQUIRE(add);
    model::Diagram d;
    CHECK((*add)->Commit(d));
  }
  DOCTEST_TEST_CASE("commands::Command::Undo") {
    auto cmd = Split("class add x");
    auto add = commands::Command::From(cmd);
    REQUIRE(add);
    model::Diagram d;
    CHECK_FALSE((*add)->Undo(d));
    CHECK((*add)->Commit(d));
    CHECK((*add)->Undo(d));
    CHECK(d.GetClasses().empty());
    cmd = Split("list all");
    auto list = commands::Command::From(cmd);
    REQUIRE(list);
    CHECK((*add)->Undo(d));
    CHECK((*add)->Commit(d));
    CHECK((*add)->Undo(d));
  }
}
