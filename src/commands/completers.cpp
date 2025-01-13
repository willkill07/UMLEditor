#include "completers.hpp"

#include "model/class.hpp"
#include "model/diagram.hpp"
#include "model/field.hpp"
#include "model/method.hpp"
#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "model/relationship.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <functional>
#include <ranges>

namespace commands {

[[nodiscard]] std::vector<std::string> ClassCompleter::Candidates() const {
  return diagram.get().GetClassNames();
}

[[nodiscard]] Result<Iter<model::Class>> ClassCompleter::Get() const {
  return diagram.get().GetClass(name);
}

[[nodiscard]] std::vector<std::string> FieldCompleter::Candidates() const {
  return iter
      .transform([](auto i) {
        return i->Fields() | std::views::transform(&model::Field::Name) | std::ranges::to<std::vector>();
      })
      .value_or(std::vector<std::string>{});
}

[[nodiscard]] Result<Iter<model::Field>> FieldCompleter::Get() const {
  return iter.and_then([&](auto i) { return i->GetReadOnlyField(name); });
}

[[nodiscard]] std::vector<std::string> MethodCompleter::Candidates() const {
  return iter
      .transform([](auto i) {
        return i->Methods() | std::views::transform(&model::Method::ToSignatureString) | std::ranges::to<std::vector>();
      })
      .value_or(std::vector<std::string>{});
}

[[nodiscard]] Result<Iter<model::Method>> MethodCompleter::Get() const {
  return model::MethodSignature::FromString(signature).and_then(
      [&](auto sig) { return iter.and_then([&](auto i) { return i->GetReadOnlyMethodFromSignature(sig); }); });
}

[[nodiscard]] std::vector<std::string> ParameterCompleter::Candidates() const {
  return iter
      .transform([](auto i) {
        return i->Parameters() | std::views::transform(&model::Parameter::Name) | std::ranges::to<std::vector>();
      })
      .value_or(std::vector<std::string>{});
}

[[nodiscard]] Result<Iter<model::Parameter>> ParameterCompleter::Get() const {
  return iter.and_then([&](auto i) { return i->GetReadOnlyParameter(name); });
}

[[nodiscard]] std::vector<std::string> RelationshipSourceCompleter::Candidates() const {
  std::vector sources = diagram.get().GetRelationships() | std::views::transform(&model::Relationship::Source) |
                        std::ranges::to<std::vector>();
  std::ranges::sort(sources);
  auto r = std::ranges::unique(sources);
  sources.erase(r.begin(), r.end());
  return sources;
}

[[nodiscard]] std::vector<std::string> RelationshipDestinationCompleter::Candidates() const {
  return diagram.get().GetRelationships() | std::views::filter([&](auto const& r) { return r.Source() == source; }) |
         std::views::transform(&model::Relationship::Destination) | std::ranges::to<std::vector>();
}

[[nodiscard]] Result<Iter<model::Relationship>> RelationshipDestinationCompleter::Get() const {
  return diagram.get().GetReadOnlyRelationship(source, dest);
}

[[nodiscard]] std::vector<std::string> RelationshipTypeCompleter::Candidates() const {
  return {"Aggregation", "Composition", "Inheritance", "Realization"};
}

} // namespace commands

DOCTEST_TEST_SUITE("commands::Completers") {
  DOCTEST_TEST_CASE("commands::ClassCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a1"));
    REQUIRE(d.AddClass("a2"));
    REQUIRE(d.AddClass("a3"));
    REQUIRE(d.AddClass("b1"));
    REQUIRE(d.AddClass("b2"));
    [[maybe_unused]] commands::ClassCompleter c{.diagram = std::cref(d), .name = "a1"};
    CHECK(std::ranges::contains(c.Candidates(), "a1"));
    CHECK(std::ranges::contains(c.Candidates(), "a2"));
    CHECK(std::ranges::contains(c.Candidates(), "a3"));
    CHECK(std::ranges::contains(c.Candidates(), "b1"));
    CHECK(std::ranges::contains(c.Candidates(), "b2"));
    REQUIRE(c.Get());
    CHECK_EQ(c.Get().value()->Name(), "a1");
  }
  DOCTEST_TEST_CASE("commands::FieldCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    auto res = d.GetClass("a");
    REQUIRE(res);
    auto a = res.value();
    REQUIRE(a->AddField("x", "int"));
    REQUIRE(a->AddField("y", "str"));
    REQUIRE(a->AddField("z", "any"));
    commands::FieldCompleter c{.iter = a, .name = "x"};
    CHECK(std::ranges::contains(c.Candidates(), "x"));
    CHECK(std::ranges::contains(c.Candidates(), "y"));
    CHECK(std::ranges::contains(c.Candidates(), "z"));
    REQUIRE(c.Get());
    CHECK_EQ(c.Get().value()->Name(), "x");
  }
  DOCTEST_TEST_CASE("commands::MethodCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    auto res = d.GetClass("a");
    REQUIRE(res);
    auto a = res.value();
    REQUIRE(a->AddMethod("f", "void", {}));
    REQUIRE(a->AddMethod("f", "int", *model::Parameter::MultipleFromString("a:int")));
    REQUIRE(a->AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:str")));
    commands::MethodCompleter c{.iter = a, .signature = "f(int)"};
    CHECK(std::ranges::contains(c.Candidates(), "f()"));
    CHECK(std::ranges::contains(c.Candidates(), "f(int)"));
    CHECK(std::ranges::contains(c.Candidates(), "f(int,str)"));
    REQUIRE(c.Get());
    CHECK_EQ(c.Get().value()->Name(), "f");
    CHECK_EQ(c.Get().value()->ReturnType(), "int");
    CHECK_EQ(c.Get().value()->Parameters().size(), 1);
  }
  DOCTEST_TEST_CASE("commands::ParameterCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    auto res = d.GetClass("a");
    REQUIRE(res);
    auto a = res.value();
    REQUIRE(a->AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:str,c:any")));
    auto res2 = a->GetMethodFromSignature(*model::MethodSignature::FromString("f(int,str,any)"));
    REQUIRE(res2);
    auto m = res2.value();
    commands::ParameterCompleter c{.iter = m, .name = "b"};
    CHECK(std::ranges::contains(c.Candidates(), "a"));
    CHECK(std::ranges::contains(c.Candidates(), "b"));
    CHECK(std::ranges::contains(c.Candidates(), "c"));
    REQUIRE(c.Get());
    CHECK_EQ(c.Get().value()->Name(), "b");
    CHECK_EQ(c.Get().value()->Type(), "str");
  }
  DOCTEST_TEST_CASE("commands::RelationshipSourceCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddClass("c"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("a", "c", model::RelationshipType::Inheritance));
    REQUIRE(d.AddRelationship("b", "c", model::RelationshipType::Composition));
    [[maybe_unused]] commands::RelationshipSourceCompleter c{.diagram = std::cref(d), .source = "a"};
    CHECK(std::ranges::contains(c.Candidates(), "a"));
    CHECK(std::ranges::contains(c.Candidates(), "b"));
    CHECK_FALSE(std::ranges::contains(c.Candidates(), "c"));
  }
  DOCTEST_TEST_CASE("commands::RelationshipDestinationCompleter") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddClass("c"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("a", "c", model::RelationshipType::Inheritance));
    REQUIRE(d.AddRelationship("b", "c", model::RelationshipType::Composition));
    [[maybe_unused]] commands::RelationshipDestinationCompleter c{.diagram = std::cref(d), .source = "a", .dest = "b"};
    CHECK_FALSE(std::ranges::contains(c.Candidates(), "a"));
    CHECK(std::ranges::contains(c.Candidates(), "b"));
    CHECK(std::ranges::contains(c.Candidates(), "c"));
    REQUIRE(c.Get());
    CHECK_EQ(c.Get().value()->Source(), "a");
    CHECK_EQ(c.Get().value()->Destination(), "b");
  }
  DOCTEST_TEST_CASE("commands::RelationshipTypeCompleter") {
    [[maybe_unused]] commands::RelationshipTypeCompleter c{};
    CHECK_EQ(c.Candidates().size(), 4);
    CHECK(std::ranges::contains(c.Candidates(), "Aggregation"));
    CHECK(std::ranges::contains(c.Candidates(), "Inheritance"));
    CHECK(std::ranges::contains(c.Candidates(), "Composition"));
    CHECK(std::ranges::contains(c.Candidates(), "Realization"));
  }
}
