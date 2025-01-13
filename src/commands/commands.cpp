#include "commands.hpp"

#include "model/diagram.hpp"
#include "model/method.hpp"
#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "model/relationship_type.hpp"
#include "timeline.hpp"
#include "utils/io_context.hpp"
#include "utils/utils.hpp"

#include <cstdio>
#include <unistd.h>

#include <doctest/doctest.h>

#include <functional>
#include <print>

namespace commands {

Result<void> LoadCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::Load, std::ref(diagram)), args);
}

Result<void> SaveCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::Save, std::ref(diagram)), args);
}

Result<void> ListAllCommand::Execute(model::Diagram& diagram) const {
  std::println(stdout, "{:cr}", diagram);
  return {};
}

Result<void> ListClassesCommand::Execute(model::Diagram& diagram) const {
  std::println(stdout, "{:c}", diagram);
  return {};
}

Result<void> ListRelationshipsCommand::Execute(model::Diagram& diagram) const {
  std::println(stdout, "{:r}", diagram);
  return {};
}

Result<void> ListClassCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls) { return diagram.GetClass(cls).transform([](auto c) { std::print("{}", *c); }); },
      args);
}

Result<void> ExitCommand::Execute(model::Diagram&) const {
  return {};
}

Result<void> HelpCommand::Execute(model::Diagram&) const {
  for (auto line : commands::CommandStrings) {
    std::println(stdout, "{}", line);
  }
  return {};
}

Result<void> UndoCommand::Execute(model::Diagram& diagram) const {
  return Timeline::GetInstance().Undo().and_then([&](auto&& cmd) { return cmd->Undo(diagram); });
}

Result<void> RedoCommand::Execute(model::Diagram& diagram) const {
  return Timeline::GetInstance().Redo().and_then([&](auto&& cmd) { return cmd->Execute(diagram); });
}

Result<void> AddClassCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::AddClass, std::ref(diagram)), args);
}

Result<void> RemoveClassCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::DeleteClass, std::ref(diagram)), args);
}

Result<void> RenameClassCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::RenameClass, std::ref(diagram)), args);
}

Result<void> MoveClassCommand::Execute(model::Diagram& diagram) const {
  return std::apply([&](std::string_view cls,
                        int x,
                        int y) { return diagram.GetClass(cls).transform([&](auto c) { c->Move(x, y); }); },
                    args);
}

Result<void> AddFieldCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, std::string_view name, std::string_view type) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->AddField(name, type); });
      },
      args);
}

Result<void> RemoveFieldCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, std::string_view field) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->DeleteField(field); });
      },
      args);
}

Result<void> RenameFieldCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, std::string_view field, std::string_view name) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->RenameField(field, name); });
      },
      args);
}

Result<void> RetypeFieldCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, std::string_view field, std::string_view type) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->GetField(field); }).and_then([&](auto f) {
          return f->ChangeType(type);
        });
      },
      args);
}

Result<void> AddMethodCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::Method const& definition) {
        return diagram.GetClass(cls).and_then(
            [&](auto c) { return c->AddMethod(definition.Name(), definition.ReturnType(), definition.Parameters()); });
      },
      args);
}

Result<void> RemoveMethodCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->DeleteMethod(sig); });
      },
      args);
}

Result<void> RenameMethodCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view name) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->RenameMethod(sig, name); });
      },
      args);
}

Result<void> ChangeReturnTypeCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view type) {
        return diagram.GetClass(cls)
            .and_then([&](auto c) { return c->GetMethodFromSignature(sig); })
            .and_then([&](auto m) { return m->ChangeReturnType(type); });
      },
      args);
}

Result<void> AddParameterCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view param, std::string_view type) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->AddParameter(sig, param, type); });
      },
      args);
}

Result<void> RemoveParameterCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view param) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->DeleteParameter(sig, param); });
      },
      args);
}

Result<void> RenameParameterCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view param, std::string_view name) {
        return diagram.GetClass(cls)
            .and_then([&](auto c) { return c->GetMethodFromSignature(sig); })
            .and_then([&](auto m) { return m->RenameParameter(param, name); });
      },
      args);
}

Result<void> RetypeParameterCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::string_view param, std::string_view type) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->ChangeParameterType(sig, param, type); });
      },
      args);
}

Result<void> ClearParametersCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->DeleteParameters(sig); });
      },
      args);
}

Result<void> SetParametersCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view cls, model::MethodSignature const& sig, std::vector<model::Parameter> const& params) {
        return diagram.GetClass(cls).and_then([&](auto c) { return c->ChangeParameters(sig, params); });
      },
      args);
}

Result<void> AddRelationshipCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::AddRelationship, std::ref(diagram)), args);
}

Result<void> RemoveRelationshipCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::DeleteRelationship, std::ref(diagram)), args);
}

Result<void> ChangeSourceCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::ChangeRelationshipSource, std::ref(diagram)), args);
}

Result<void> ChangeDestinationCommand::Execute(model::Diagram& diagram) const {
  return std::apply(std::bind_front(&model::Diagram::ChangeRelationshipDestination, std::ref(diagram)), args);
}

Result<void> ChangeTypeCommand::Execute(model::Diagram& diagram) const {
  return std::apply(
      [&](std::string_view src, std::string_view dst, model::RelationshipType type) {
        return diagram.GetRelationship(src, dst).transform([&](auto r) { return r->ChangeType(type); });
      },
      args);
}

} // namespace commands

DOCTEST_TEST_SUITE("commands") {
  DOCTEST_TEST_CASE("commands::LoadCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::LoadCommand>(std::tuple{"file.txt"});
    CHECK_FALSE(cmd->Commit(d));
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::SaveCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::SaveCommand>(std::tuple{"/invalid.txt"});
    CHECK_FALSE(cmd->Commit(d));
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::ListAllCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ListAllCommand>(std::tuple<>{});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Inheritance));
    [[maybe_unused]] Result<void> res;
    ENABLE_IF_TEST({
      IOContext ctx;
      res = cmd->Commit(d);
      std::ignore = fflush(stdout);
    });
    CHECK(res);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::ListClassesCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ListClassesCommand>(std::tuple<>{});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    [[maybe_unused]] Result<void> res;
    ENABLE_IF_TEST({
      IOContext ctx;
      res = cmd->Commit(d);
      std::ignore = fflush(stdout);
    });
    CHECK(res);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::ListRelationshipsCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ListRelationshipsCommand>(std::tuple<>{});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Inheritance));
    [[maybe_unused]] Result<void> res;
    ENABLE_IF_TEST({
      IOContext ctx;
      res = cmd->Commit(d);
      std::ignore = fflush(stdout);
    });
    CHECK(res);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::ListClassCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ListClassCommand>(std::tuple{"a"});
    REQUIRE(d.AddClass("a"));
    [[maybe_unused]] Result<void> res;
    ENABLE_IF_TEST({
      IOContext ctx;
      res = cmd->Commit(d);
      std::ignore = fflush(stdout);
    });
    CHECK(res);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::ExitCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ExitCommand>(std::tuple<>{});
    CHECK(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::HelpCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::HelpCommand>(std::tuple<>{});
    CHECK(cmd->Undo(d));
    [[maybe_unused]] Result<void> res;
    ENABLE_IF_TEST({
      IOContext ctx;
      res = cmd->Commit(d);
      std::ignore = fflush(stdout);
    });
    CHECK(res);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::UndoCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::UndoCommand>(std::tuple<>{});
    CHECK(cmd->Undo(d));
    std::ignore = cmd->Commit(d);
    CHECK(cmd->Undo(d));
  }
  DOCTEST_TEST_CASE("commands::RedoCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RedoCommand>(std::tuple<>{});
    CHECK(cmd->Undo(d));
    std::ignore = cmd->Commit(d);
    CHECK(cmd->Undo(d));
  }

  DOCTEST_TEST_CASE("commands::AddClassCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::AddClassCommand>(std::tuple{"a"});
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    REQUIRE(d.GetClass("a"));
    CHECK(cmd->Undo(d));
    REQUIRE_FALSE(d.GetClass("a"));
  }
  DOCTEST_TEST_CASE("commands::RemoveClassCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RemoveClassCommand>(std::tuple{"a"});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    REQUIRE_FALSE(d.GetClass("a"));
    CHECK(cmd->Undo(d));
    REQUIRE(d.GetClass("a"));
  }
  DOCTEST_TEST_CASE("commands::RenameClassCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RenameClassCommand>(std::tuple{"a", "b"});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    REQUIRE(d.GetClass("b"));
    CHECK(cmd->Undo(d));
    REQUIRE(d.GetClass("a"));
  }
  DOCTEST_TEST_CASE("commands::MoveClassCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::MoveClassCommand>(std::tuple{"a", 420, 69});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Position(), model::Point{420, 69});
    CHECK(cmd->Undo(d));
    CHECK_NE(d.GetClasses().front().Position(), model::Point{420, 69});
  }

  DOCTEST_TEST_CASE("commands::AddFieldCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::AddFieldCommand>(std::tuple{"a", "x", "int"});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "x");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "int");
    CHECK(cmd->Undo(d));
    CHECK(d.GetClasses().front().Fields().empty());
  }
  DOCTEST_TEST_CASE("commands::RemoveFieldCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RemoveFieldCommand>(std::tuple{"a", "x"});
    REQUIRE(d.AddClass("a"));
    REQUIRE((*d.GetClass("a"))->AddField("x", "int"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK(d.GetClasses().front().Fields().empty());
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "x");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "int");
  }
  DOCTEST_TEST_CASE("commands::RenameFieldCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RenameFieldCommand>(std::tuple{"a", "x", "y"});
    REQUIRE(d.AddClass("a"));
    REQUIRE((*d.GetClass("a"))->AddField("x", "int"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "y");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "int");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "x");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "int");
  }
  DOCTEST_TEST_CASE("commands::RetypeFieldCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RetypeFieldCommand>(std::tuple{"a", "x", "str"});
    REQUIRE(d.AddClass("a"));
    REQUIRE((*d.GetClass("a"))->AddField("x", "int"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "x");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "str");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Fields().front().Name(), "x");
    CHECK_EQ(d.GetClasses().front().Fields().front().Type(), "int");
  }

  DOCTEST_TEST_CASE("commands::AddMethodCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd =
        std::make_unique<commands::AddMethodCommand>(std::tuple{"a", *model::Method::FromString("f(x:int)->str")});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Name(), "f");
    CHECK_EQ(d.GetClasses().front().Methods().front().ReturnType(), "str");
    CHECK(cmd->Undo(d));
    CHECK(d.GetClasses().front().Methods().empty());
  }
  DOCTEST_TEST_CASE("commands::RemoveMethodCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RemoveMethodCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)")});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK(d.GetClasses().front().Methods().empty());
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Name(), "f");
  }
  DOCTEST_TEST_CASE("commands::RenameMethodCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RenameMethodCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "g"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Name(), "g");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Name(), "f");
  }
  DOCTEST_TEST_CASE("commands::ChangeReturnTypeCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ChangeReturnTypeCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "int"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().ReturnType(), "int");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().ReturnType(), "void");
  }

  DOCTEST_TEST_CASE("commands::AddParameterCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::AddParameterCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "c", "any"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "c");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "b");
  }
  DOCTEST_TEST_CASE("commands::RemoveParameterCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RemoveParameterCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "b"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "a");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "b");
  }
  DOCTEST_TEST_CASE("commands::RenameParameterCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RenameParameterCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "b", "c"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "c");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Name(), "b");
  }
  DOCTEST_TEST_CASE("commands::RetypeParameterCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RetypeParameterCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)"), "b", "int"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Type(), "int");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().back().Type(), "str");
  }

  DOCTEST_TEST_CASE("commands::ClearParametersCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ClearParametersCommand>(
        std::tuple{"a", *model::MethodSignature::FromString("f(int,str)")});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK(d.GetClasses().front().Methods().front().Parameters().empty());
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().size(), 2);
  }
  DOCTEST_TEST_CASE("commands::SetParametersCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::SetParametersCommand>(
        std::tuple{"a",
                   *model::MethodSignature::FromString("f(int,str)"),
                   *model::Parameter::MultipleFromString("a:int,b:int,c:int,d:int")});
    REQUIRE(d.AddClass("a"));
    REQUIRE(
        d.GetClass("a").value()->AddMethod("f", "void", model::Parameter::MultipleFromString("a:int,b:str").value()));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().size(), 4);
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetClasses().front().Methods().front().Parameters().size(), 2);
  }

  DOCTEST_TEST_CASE("commands::AddRelationshipCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd =
        std::make_unique<commands::AddRelationshipCommand>(std::tuple{"a", "a", model::RelationshipType::Composition});
    REQUIRE(d.AddClass("a"));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetRelationships().size(), 1);
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetRelationships().size(), 0);
  }
  DOCTEST_TEST_CASE("commands::RemoveRelationshipCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::RemoveRelationshipCommand>(std::tuple{"a", "a"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Composition));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetRelationships().size(), 0);
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetRelationships().size(), 1);
  }
  DOCTEST_TEST_CASE("commands::ChangeSourceCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ChangeSourceCommand>(std::tuple{"a", "a", "b"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Composition));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetRelationships().front().Source(), "b");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetRelationships().front().Source(), "a");
  }
  DOCTEST_TEST_CASE("commands::ChangeDestinationCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd = std::make_unique<commands::ChangeDestinationCommand>(std::tuple{"a", "a", "b"});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Composition));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetRelationships().front().Destination(), "b");
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetRelationships().front().Destination(), "a");
  }
  DOCTEST_TEST_CASE("commands::ChangeTypeCommand") {
    [[maybe_unused]] model::Diagram d;
    auto cmd =
        std::make_unique<commands::ChangeTypeCommand>(std::tuple{"a", "a", model::RelationshipType::Realization});
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Composition));
    CHECK_FALSE(cmd->Undo(d));
    CHECK(cmd->Commit(d));
    CHECK_EQ(d.GetRelationships().front().Type(), model::RelationshipType::Realization);
    CHECK(cmd->Undo(d));
    CHECK_EQ(d.GetRelationships().front().Type(), model::RelationshipType::Composition);
  }
}
