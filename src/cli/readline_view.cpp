#include "cli/readline_controller.hpp"
#include "commands/base_commands.hpp"
#include "commands/timeline.hpp"
#include "model/diagram.hpp"
#include "utils/io_context.hpp"

#include <doctest/doctest.h>

namespace cli {

int CLI() {
  model::Diagram& diagram = model::Diagram::GetInstance();
  ReadlineInterface repl{"UML> "};
  while (repl.ReadCommand()) {
    auto tokens = repl.GetTokenizedCommand();
    auto res = commands::Command::From(tokens).and_then([&]<typename Cmd>(Cmd&& cmd) {
      repl.AddCommandToHistory();
      return cmd->Commit(diagram).transform([&] { commands::Timeline::GetInstance().Add(std::forward<Cmd>(cmd)); });
    });
    if (res.has_value()) {
      if (tokens[0] == "exit") {
        break;
      }
    } else {
      repl.DisplayMessage(res.error());
    }
  }
  return 0;
}

} // namespace cli

DOCTEST_TEST_SUITE("cli") {
  DOCTEST_TEST_CASE("cli::CLI") {
    [[maybe_unused]] int return_code{0};
    [[maybe_unused]] std::string err, out;
    ENABLE_IF_TEST({
      IOContext ctx;
      ctx.StdIn(R"(invalid command
class add a
class add b
relationship add a b Composition
list all
exit
)");
      return_code = cli::CLI();
      err = ctx.StdErr();
      out = ctx.StdOut();
    })
    CHECK_EQ(return_code, 0);
    CHECK(std::string_view{err}.starts_with("Invalid command"));
    CHECK_FALSE(out.empty());

    [[maybe_unused]] auto& d = model::Diagram::GetInstance();
    REQUIRE_EQ(d.GetClasses().size(), 2);
    CHECK_EQ(d.GetClasses().front().Name(), "a");
    CHECK_EQ(d.GetClasses().back().Name(), "b");
    REQUIRE_EQ(d.GetRelationships().size(), 1);
    CHECK_EQ(d.GetRelationships().front().Source(), "a");
    CHECK_EQ(d.GetRelationships().front().Destination(), "b");
    CHECK_EQ(d.GetRelationships().front().Type(), model::RelationshipType::Composition);
    CHECK(d.DeleteClass("a"));
    CHECK(d.DeleteClass("b"));
    REQUIRE(d.GetClasses().empty());
    REQUIRE(d.GetRelationships().empty());
  }
}
