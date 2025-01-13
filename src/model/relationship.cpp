#include "relationship.hpp"

#include "model/checking.hpp"
#include "model/relationship_type.hpp"
#include "nlohmann/json_fwd.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace model {

// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann ::json& json, const Relationship& r) {
  json["source"] = r.Source();
  json["destination"] = r.Destination();
  json["type"] = r.Type();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann ::json& json, Relationship& r) {
  auto res = Relationship::From(json.at("source").get<std::string>(),
                                json.at("destination").get<std::string>(),
                                json.at("type").get<RelationshipType>())
                 .transform([&](Relationship rel) { r = std::move(rel); });
  if (not res) {
    throw std::runtime_error{res.error()};
  }
}

Relationship::Relationship() = default;

Result<Relationship> Relationship::From(std::string_view source, std::string_view destination, RelationshipType type) {
  return Check<ValidType>(source, "class name")
      .and_then([&] { return Check<ValidType>(destination, "class name"); })
      .transform([&] {
        Relationship rel;
        rel.source_ = source;
        rel.destination_ = destination;
        rel.type_ = type;
        return rel;
      });
}

std::string const& Relationship::Source() const noexcept {
  return source_;
}
std::string const& Relationship::Destination() const noexcept {
  return destination_;
}
RelationshipType Relationship::Type() const noexcept {
  return type_;
}

void Relationship::ChangeType(RelationshipType new_type) {
  type_ = new_type;
}

Result<void> Relationship::ChangeSource(std::string_view new_source) {
  return Check<ValidType>(new_source, "class name").transform([&] { source_ = new_source; });
}

Result<void> Relationship::ChangeDestination(std::string_view new_destination) {
  return Check<ValidType>(new_destination, "class name").transform([&] { destination_ = new_destination; });
}

bool Relationship::operator==(Relationship const& other) const noexcept {
  return (source_ == other.source_) and (destination_ == other.destination_);
}

std::strong_ordering Relationship::operator<=>(Relationship const& other) const noexcept {
  if (auto res = (source_ <=> other.source_); res == 0) {
    return (destination_ <=> other.destination_);
  } else {
    return res;
  }
}

} // namespace model

DOCTEST_TEST_SUITE("model::Relationship") {
  DOCTEST_TEST_CASE("model::Relationship.From") {
    Result<model::Relationship> rel;
    rel = model::Relationship::From("A", "B", model::RelationshipType::Aggregation);
    REQUIRE(rel.has_value());
    CHECK_EQ(rel->Source(), "A");
    CHECK_EQ(rel->Destination(), "B");
    CHECK_EQ(rel->Type(), model::RelationshipType::Aggregation);
    rel = model::Relationship::From(" ", "B", model::RelationshipType::Realization);
    REQUIRE_FALSE(rel.has_value());
    rel = model::Relationship::From("A", " ", model::RelationshipType::Realization);
    REQUIRE_FALSE(rel.has_value());
  }
  DOCTEST_TEST_CASE("model::Relationship.Json") {
    model::Relationship rel{};
    nlohmann::json json;
    json = nlohmann::json{};
    json["source"] = "A";
    json["destination"] = "B";
    json["type"] = "Aggregation";
    REQUIRE_NOTHROW(rel = json);
    CHECK_EQ(rel.Source(), "A");
    CHECK_EQ(rel.Destination(), "B");
    CHECK_EQ(rel.Type(), model::RelationshipType::Aggregation);
    json["source"] = " ";
    CHECK_THROWS(rel = json);
    json["source"] = "A";
    json["destination"] = " ";
    CHECK_THROWS(rel = json);
    json["destination"] = "B";
    json["type"] = "invalid";
    CHECK_THROWS(rel = json);
    auto r = model::Relationship::From("A", "B", model::RelationshipType::Realization);
    REQUIRE(r.has_value());
    json = r.value();
    CHECK_EQ(json.at("source"), "A");
    CHECK_EQ(json.at("destination"), "B");
    CHECK_EQ(json.at("type"), "Realization");
  }
  DOCTEST_TEST_CASE("model::Relationship.Change") {
    auto r = model::Relationship::From("A", "B", model::RelationshipType::Realization);
    CHECK_EQ(r->Type(), model::RelationshipType::Realization);
    r->ChangeType(model::RelationshipType::Composition);
    CHECK_EQ(r->Type(), model::RelationshipType::Composition);
    CHECK_EQ(r->Source(), "A");
    CHECK(r->ChangeSource("C").has_value());
    CHECK_EQ(r->Source(), "C");
    CHECK_FALSE(r->ChangeSource("  ").has_value());
    CHECK_EQ(r->Source(), "C");

    CHECK_EQ(r->Destination(), "B");
    CHECK(r->ChangeDestination("D").has_value());
    CHECK_EQ(r->Destination(), "D");
    CHECK_FALSE(r->ChangeDestination("  ").has_value());
    CHECK_EQ(r->Destination(), "D");
  }
  DOCTEST_TEST_CASE("model::Relationship.Compare") {
    auto r1 = model::Relationship::From("A", "B", model::RelationshipType::Realization);
    auto r2 = model::Relationship::From("B", "B", model::RelationshipType::Realization);
    auto r3 = model::Relationship::From("A", "A", model::RelationshipType::Realization);
    auto r4 = model::Relationship::From("A", "A", model::RelationshipType::Composition);
    CHECK_EQ(*r1, *r1);
    CHECK_LE(*r1, *r1);
    CHECK_GE(*r1, *r1);
    CHECK_NE(*r1, *r2);
    CHECK_LT(*r1, *r2);
    CHECK_GT(*r2, *r1);
    CHECK_GE(*r1, *r1);
    CHECK_LT(*r3, *r1);
    CHECK_LE(*r3, *r1);
    CHECK_EQ(*r3, *r4);
    CHECK_LE(*r3, *r4);
    CHECK_GE(*r3, *r4);
  }
  DOCTEST_TEST_CASE("model::Relationship.Format") {
    auto r = model::Relationship::From("A", "B", model::RelationshipType::Realization);
    REQUIRE(r.has_value());
    CHECK_EQ(std::format("{}", r.value()), "A -> B (Realization)");
  }
}
