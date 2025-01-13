#include "diagram.hpp"

#include "model/checking.hpp"
#include "model/class.hpp"
#include "model/relationship.hpp"
#include "model/relationship_type.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace model {

// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann ::json& json, const Diagram& d) {
  json["classes"] = d.GetClasses();
  json["relationships"] = d.GetRelationships();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann ::json& json, Diagram& d) {
  json.at("classes").get_to(d.classes_);
  json.at("relationships").get_to(d.relationships_);
  auto res = Unique(d.classes_, "class")
                 .and_then([&] { return Unique(d.relationships_, "relationship"); })
                 .and_then([&]() -> Result<Diagram> {
                   std::vector<std::string> rel_classes;
                   {
                     rel_classes.reserve(d.relationships_.size() * 2);
                     auto i = std::back_inserter(rel_classes);
                     i = std::ranges::transform(d.relationships_, i, &Relationship::Source).out;
                     std::ranges::transform(d.relationships_, i, &Relationship::Destination);
                     std::ranges::sort(rel_classes);
                     auto dups = std::ranges::unique(rel_classes);
                     rel_classes.erase(dups.begin(), dups.end());
                   }
                   if (not std::ranges::includes(d.classes_, rel_classes, {}, &Class::Name)) {
                     return std::unexpected{"Relationship(s) contain nonexistent class(es)"};
                   } else {
                     return {};
                   }
                 });
  if (not res) {
    throw std::invalid_argument{res.error()};
  }
}

Result<std::vector<Class>::iterator> Diagram::GetClass(std::string_view name) {
  return Check<ValidType>(name, "class name").and_then([&]() -> Result<std::vector<Class>::iterator> {
    if (auto i = std::ranges::find(classes_, name, &Class::Name); i != classes_.end()) {
      return i;
    } else {
      return std::unexpected{std::format("class '{}' does not exist", name)};
    }
  });
}

Result<std::vector<Class>::const_iterator> Diagram::GetClass(std::string_view name) const {
  return Check<ValidType>(name, "class name").and_then([&]() -> Result<std::vector<Class>::const_iterator> {
    if (auto i = std::ranges::find(classes_, name, &Class::Name); i != classes_.end()) {
      return i;
    } else {
      return std::unexpected{std::format("class '{}' does not exist", name)};
    }
  });
}

Result<std::vector<Relationship>::iterator> Diagram::GetRelationship(std::string_view src, std::string_view dst) {
  return Check<ValidType>(src, "class name").and_then([&] {
    return Check<ValidType>(dst, "class name").and_then([&]() -> Result<std::vector<Relationship>::iterator> {
      if (auto i = std::ranges::find_if(
              relationships_, [&](Relationship const& r) { return (r.Source() == src) and (r.Destination() == dst); });
          i != relationships_.end()) {
        return i;
      } else {
        return std::unexpected{std::format("relationship between '{}' and '{}' does not exist", src, dst)};
      }
    });
  });
}

Result<std::vector<Relationship>::const_iterator> Diagram::GetReadOnlyRelationship(std::string_view src,
                                                                                   std::string_view dst) const {
  return Check<ValidType>(src, "class name").and_then([&] {
    return Check<ValidType>(dst, "class name").and_then([&]() -> Result<std::vector<Relationship>::const_iterator> {
      if (auto i = std::ranges::find_if(
              relationships_, [&](Relationship const& r) { return (r.Source() == src) and (r.Destination() == dst); });
          i != relationships_.end()) {
        return i;
      } else {
        return std::unexpected{std::format("relationship between '{}' and '{}' does not exist", src, dst)};
      }
    });
  });
}

Diagram& Diagram::GetInstance() noexcept {
  static Diagram diagram;
  return diagram;
}

Result<void> Diagram::AddClass(std::string_view name) {
  if (not GetClass(name)) {
    return Class::From(name).transform([&](Class c) { classes_.push_back(std::move(c)); });
  } else {
    return std::unexpected(std::format("Class '{}' cannot be added because it already exists", name));
  }
}

Result<void> Diagram::DeleteClass(std::string_view name) {
  return GetClass(name).transform([&](auto c) {
    std::erase_if(relationships_, [&](Relationship const& r) { return r.Source() == name or r.Destination() == name; });
    classes_.erase(c);
  });
}

Result<void> Diagram::RenameClass(std::string_view old_name, std::string_view new_name) {
  return GetClass(old_name).and_then([&](auto c) -> Result<void> {
    if (not GetClass(new_name)) {
      return c->Rename(new_name).transform([&] {
        std::ranges::sort(classes_);
        for (Relationship& r : relationships_) {
          if (r.Source() == old_name) {
            std::ignore = r.ChangeSource(new_name);
          }
          if (r.Destination() == old_name) {
            std::ignore = r.ChangeDestination(new_name);
          }
        }
      });
    } else {
      return std::unexpected{"the new class already exists"};
    }
  });
}

Result<void> Diagram::AddRelationship(std::string_view source, std::string_view destination, RelationshipType type) {
  return GetClass(source).and_then([&](auto&&) { return GetClass(destination); }).and_then([&](auto&&) -> Result<void> {
    if (not GetRelationship(source, destination)) {
      return Relationship::From(source, destination, type).transform([&](Relationship r) {
        relationships_.push_back(std::move(r));
        std::ranges::sort(relationships_);
      });
    } else {
      return std::unexpected{"Cannot add relationship because it already exists"};
    }
  });
}

Result<void> Diagram::DeleteRelationship(std::string_view source, std::string_view destination) {
  return GetRelationship(source, destination).transform([&](auto r) { relationships_.erase(r); });
}

Result<void>
Diagram::ChangeRelationshipSource(std::string_view source, std::string_view destination, std::string_view new_source) {
  return GetRelationship(source, destination).and_then([&](auto r) -> Result<void> {
    if (not GetRelationship(new_source, destination)) {
      return GetClass(new_source).and_then([&](auto&&) {
        return r->ChangeSource(new_source).transform([&] { std::ranges::sort(relationships_); });
      });
    } else {
      return std::unexpected{std::format("a relationship between {} and {} already exists", new_source, destination)};
    }
  });
}

Result<void> Diagram::ChangeRelationshipDestination(std::string_view source,
                                                    std::string_view destination,
                                                    std::string_view new_destination) {
  return GetRelationship(source, destination).and_then([&](auto r) -> Result<void> {
    if (not GetRelationship(source, new_destination)) {
      return GetClass(new_destination).and_then([&](auto&&) {
        return r->ChangeDestination(new_destination).transform([&] { std::ranges::sort(relationships_); });
      });
    } else {
      return std::unexpected{std::format("a relationship between {} and {} already exists", source, new_destination)};
    }
  });
}

Result<void> Diagram::Load(std::string_view file_name) {
  try {
    std::filesystem::path file{file_name};
    std::filesystem::path const resolved = std::filesystem::absolute(file);
    std::ifstream ifs{resolved};
    nlohmann::from_json(nlohmann::json::parse(ifs), *this);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected{std::format("Error: {}", e.what())};
  }
}

Result<void> Diagram::Save(std::string_view file_name) {
  try {
    std::filesystem::path file{file_name};
    std::filesystem::path const resolved = std::filesystem::absolute(file);
    std::ofstream ofs{resolved};
    nlohmann::json as_json = *this;
    ofs << as_json.dump(/*indent=*/2, /*indent_char=*/' ', /*ensure_ascii=*/true);
    if (not ofs.good()) {
      throw std::runtime_error{std::format("Cannot write file \"{}\"", resolved.string())};
    }
    ofs.close();
    return {};
  } catch (std::exception const& e) {
    return std::unexpected{std::format("Error: {}", e.what())};
  }
}

std::vector<std::string> Diagram::GetClassNames() const {
  return std::ranges::to<std::vector>(std::views::transform(classes_, &Class::Name));
}

std::vector<Class> const& Diagram::GetClasses() const noexcept {
  return classes_;
}

std::vector<Relationship> const& Diagram::GetRelationships() const noexcept {
  return relationships_;
}

} // namespace model

DOCTEST_TEST_SUITE("model::Diagram") {
  DOCTEST_TEST_CASE("model::Diagram.AddClass") {
    model::Diagram d;
    CHECK_FALSE(d.AddClass(" "));
    CHECK(d.AddClass("a"));
    CHECK_FALSE(d.AddClass("a"));
    CHECK(d.AddClass("b"));
    CHECK_EQ(d.GetClasses().size(), 2);
  }
  DOCTEST_TEST_CASE("model::Diagram.GetClass") {
    model::Diagram d;
    auto const& dc = d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddClass("c"));
    CHECK_FALSE(d.GetClass(" "));
    CHECK_FALSE(d.GetClass("d"));
    CHECK_FALSE(dc.GetClass(" "));
    CHECK_FALSE(dc.GetClass("d"));
    auto a = d.GetClass("a");
    REQUIRE(a);
    CHECK_EQ((*a)->Name(), "a");
    auto ac = dc.GetClass("a");
    REQUIRE(ac);
    CHECK_EQ((*ac)->Name(), "a");
  }
  DOCTEST_TEST_CASE("model::Diagram.DeleteClass") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddClass("c"));
    CHECK_FALSE(d.DeleteClass(" "));
    CHECK_FALSE(d.DeleteClass("d"));
    CHECK(d.DeleteClass("a"));
    CHECK_FALSE(d.GetClass("a"));
    CHECK(d.DeleteClass("c"));
    CHECK_FALSE(d.GetClass("c"));
    CHECK(d.DeleteClass("b"));
    CHECK_FALSE(d.GetClass("b"));
    CHECK(d.GetClasses().empty());
  }
  DOCTEST_TEST_CASE("model::Diagram.RenameClass") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddClass("c"));
    std::ignore = d.AddRelationship("a", "b", model::RelationshipType::Inheritance);
    std::ignore = d.AddRelationship("b", "a", model::RelationshipType::Composition);
    CHECK_FALSE(d.RenameClass(" ", "d"));
    CHECK_FALSE(d.RenameClass("a", "b"));
    CHECK(d.RenameClass("a", "d"));
    CHECK_EQ(d.GetClasses()[0].Name(), "b");
    CHECK(d.RenameClass("b", "e"));
    CHECK_EQ(d.GetClasses()[0].Name(), "c");
    CHECK(d.RenameClass("c", "f"));
    CHECK_EQ(d.GetClasses()[0].Name(), "d");
    CHECK(d.RenameClass("d", "a"));
    CHECK_EQ(d.GetClasses()[0].Name(), "a");
  }
  DOCTEST_TEST_CASE("model::Diagram.AddRelationship") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    CHECK_FALSE(d.AddRelationship(" ", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.AddRelationship("a", " ", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.AddRelationship("a", "d", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.AddRelationship("d", "b", model::RelationshipType::Aggregation));
    CHECK(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    REQUIRE_FALSE(d.GetRelationships().empty());
    [[maybe_unused]] auto& rel = d.GetRelationships().front();
    CHECK_EQ(rel.Source(), "a");
    CHECK_EQ(rel.Destination(), "b");
    CHECK_EQ(rel.Type(), model::RelationshipType::Aggregation);
  }
  DOCTEST_TEST_CASE("model::Diagram.GetRelationship") {
    model::Diagram d;
    [[maybe_unused]] auto const& dc = d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.GetRelationship(" ", " "));
    CHECK_FALSE(d.GetRelationship("a", " "));
    CHECK_FALSE(d.GetRelationship(" ", "b"));
    CHECK_FALSE(d.GetRelationship("b", "a"));
    CHECK_FALSE(d.GetRelationship("d", "a"));
    CHECK_FALSE(d.GetRelationship("a", "d"));
    auto rel = d.GetRelationship("a", "b");

    CHECK_FALSE(dc.GetReadOnlyRelationship(" ", " "));
    CHECK_FALSE(dc.GetReadOnlyRelationship("a", " "));
    CHECK_FALSE(dc.GetReadOnlyRelationship(" ", "b"));
    CHECK_FALSE(dc.GetReadOnlyRelationship("b", "a"));
    CHECK_FALSE(dc.GetReadOnlyRelationship("d", "a"));
    CHECK_FALSE(dc.GetReadOnlyRelationship("a", "d"));
    CHECK(dc.GetReadOnlyRelationship("a", "b"));

    REQUIRE(rel);
    CHECK_EQ((*rel)->Source(), "a");
    CHECK_EQ((*rel)->Destination(), "b");
    CHECK_EQ((*rel)->Type(), model::RelationshipType::Aggregation);
  }
  DOCTEST_TEST_CASE("model::Diagram.DeleteRelationship") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.DeleteRelationship(" ", " "));
    CHECK_FALSE(d.DeleteRelationship("a", " "));
    CHECK_FALSE(d.DeleteRelationship(" ", "b"));
    CHECK_FALSE(d.DeleteRelationship("a", "d"));
    CHECK_FALSE(d.DeleteRelationship("d", "b"));
    CHECK_FALSE(d.DeleteRelationship("b", "a"));
    CHECK(d.DeleteRelationship("a", "b"));
    CHECK(d.GetRelationships().empty());
  }
  DOCTEST_TEST_CASE("model::Diagram.ChangeRelationshipSource") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("b", "a", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("b", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.ChangeRelationshipSource(" ", " ", "a"));
    CHECK_FALSE(d.ChangeRelationshipSource("a", "b", "d"));
    CHECK_FALSE(d.ChangeRelationshipSource(" ", "b", "a"));
    CHECK_FALSE(d.ChangeRelationshipSource("a", " ", "b"));
    CHECK_FALSE(d.ChangeRelationshipSource("a", "a", "d"));
    CHECK_FALSE(d.ChangeRelationshipSource("a", "a", "b"));
    CHECK_FALSE(d.ChangeRelationshipSource("a", "a", "a"));
    CHECK(d.ChangeRelationshipSource("b", "b", "a"));
    CHECK_FALSE(d.GetRelationship("b", "b"));
    CHECK(d.GetRelationship("a", "b"));
  }
  DOCTEST_TEST_CASE("model::Diagram.ChangeRelationshipDestination") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    REQUIRE(d.AddRelationship("a", "a", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("a", "b", model::RelationshipType::Aggregation));
    REQUIRE(d.AddRelationship("b", "b", model::RelationshipType::Aggregation));
    CHECK_FALSE(d.ChangeRelationshipDestination(" ", " ", "a"));
    CHECK_FALSE(d.ChangeRelationshipDestination("b", "a", "d"));
    CHECK_FALSE(d.ChangeRelationshipDestination(" ", "a", "a"));
    CHECK_FALSE(d.ChangeRelationshipDestination("b", " ", "b"));
    CHECK_FALSE(d.ChangeRelationshipDestination("b", "b", "d"));
    CHECK_FALSE(d.ChangeRelationshipDestination("b", "b", "b"));
    CHECK_FALSE(d.ChangeRelationshipDestination("a", "a", "b"));
    CHECK(d.ChangeRelationshipDestination("b", "b", "a"));
    CHECK_FALSE(d.GetRelationship("b", "b"));
    CHECK(d.GetRelationship("b", "a"));
  }
  DOCTEST_TEST_CASE("model::Diagram.GetClassNames") {
    model::Diagram d;
    REQUIRE(d.AddClass("a"));
    REQUIRE(d.AddClass("b"));
    CHECK_EQ(d.GetClassNames(), std::vector<std::string>{"a", "b"});
  }
  DOCTEST_TEST_CASE("model::Diagram.Json") {
    DOCTEST_SUBCASE("Valid") {
      auto json = R"({
        "classes": [
          {"name": "a", "fields": [], "methods": [], "position": {"x": 0, "y": 0}},
          {"name": "b", "fields": [], "methods": [], "position": {"x": 0, "y": 0}}
        ],
        "relationships": [
          {"source": "a", "destination": "b", "type": "Aggregation"}
        ]
      })"_json;
      model::Diagram d;
      REQUIRE_NOTHROW(d = json);
      nlohmann::json j;
      REQUIRE_NOTHROW(j = d);
      CHECK_EQ(j, json);
    }
    DOCTEST_SUBCASE("Invalid") {
      auto json = R"({
        "classes": [
          {"name": "a", "fields": [], "methods": [], "position": {"x": 0, "y": 0}},
          {"name": "a", "fields": [], "methods": [], "position": {"x": 0, "y": 0}}
        ],
        "relationships": []
      })"_json;
      model::Diagram d;
      REQUIRE_THROWS(d = json);
    }
    DOCTEST_SUBCASE("Invalid") {
      auto json = R"({
        "classes": [],
        "relationships": [
          {"source": "a", "destination": "b", "type": "Aggregation"}
        ]
      })"_json;
      model::Diagram d;
      REQUIRE_THROWS(d = json);
    }
  }
  DOCTEST_TEST_CASE("model::Diagram.SaveLoad") {
    auto json = R"({
      "classes": [
        {"name": "a", "fields": [], "methods": [], "position": {"x": 0, "y": 0}},
        {"name": "b", "fields": [], "methods": [], "position": {"x": 0, "y": 0}}
      ],
      "relationships": [
        {"source": "a", "destination": "b", "type": "Aggregation"}
      ]
    })"_json;
    model::Diagram d;
    REQUIRE_NOTHROW(d = json);
    CHECK_FALSE(d.Save("/invalid-file"));
    [[maybe_unused]] auto tmp = std::filesystem::temp_directory_path() / "test.json";
    CHECK(d.Save(tmp.string()));
    model::Diagram d2;
    CHECK_FALSE(d2.Load("/invalid-file"));
    CHECK(d2.Load(tmp.string()));
    CHECK_EQ(d.GetClasses(), d2.GetClasses());
    CHECK_EQ(d.GetRelationships(), d2.GetRelationships());
  }
  DOCTEST_TEST_CASE("model::Diagram::GetInstance") {
    REQUIRE(model::Diagram::GetInstance().AddClass("a"));
    REQUIRE_FALSE(model::Diagram::GetInstance().AddClass("a"));
    REQUIRE(model::Diagram::GetInstance().DeleteClass("a"));
    REQUIRE_FALSE(model::Diagram::GetInstance().DeleteClass("a"));
  }
}
