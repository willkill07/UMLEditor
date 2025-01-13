#include "relationship_type.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <format>
#include <stdexcept>

namespace model {
// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann::json& j, RelationshipType t) {
  j = std::format("{}", t);
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann::json& j, RelationshipType& t) {
  auto res = RelationshipTypeFromString(std::string{j}).transform([&](RelationshipType type) { t = type; });
  if (not res) {
    throw std::invalid_argument{res.error()};
  }
}

Result<RelationshipType> RelationshipTypeFromString(std::string_view s) {
  if (s == "Aggregation") {
    return RelationshipType::Aggregation;
  } else if (s == "Composition") {
    return RelationshipType::Composition;
  } else if (s == "Inheritance") {
    return RelationshipType::Inheritance;
  } else if (s == "Realization") {
    return RelationshipType::Realization;
  } else {
    return std::unexpected{std::format("invalid relationship type: '{}'", s)};
  }
}

} // namespace model

DOCTEST_TEST_SUITE("model::RelationshipType") {
  DOCTEST_TEST_CASE("model::RelationshipType.Json") {
    [[maybe_unused]] std::string s;
    nlohmann::json json;
    REQUIRE_NOTHROW(json = model::RelationshipType::Aggregation);
    REQUIRE_NOTHROW(s = json);
    CHECK_EQ(s, "Aggregation");
    REQUIRE_NOTHROW(json = model::RelationshipType::Composition);
    REQUIRE_NOTHROW(s = json);
    CHECK_EQ(s, "Composition");
    REQUIRE_NOTHROW(json = model::RelationshipType::Inheritance);
    REQUIRE_NOTHROW(s = json);
    CHECK_EQ(s, "Inheritance");
    REQUIRE_NOTHROW(json = model::RelationshipType::Realization);
    REQUIRE_NOTHROW(s = json);
    CHECK_EQ(s, "Realization");
    [[maybe_unused]] model::RelationshipType t;
    json = nullptr;
    CHECK_THROWS(t = json);
    json = "invalid";
    CHECK_THROWS(t = json);
    json = std::vector<std::string>{};
    CHECK_THROWS(t = json);
    json = "Aggregation";
    REQUIRE_NOTHROW(t = json);
    CHECK_EQ(t, model::RelationshipType::Aggregation);
    json = "Composition";
    REQUIRE_NOTHROW(t = json);
    CHECK_EQ(t, model::RelationshipType::Composition);
    json = "Inheritance";
    REQUIRE_NOTHROW(t = json);
    CHECK_EQ(t, model::RelationshipType::Inheritance);
    json = "Realization";
    REQUIRE_NOTHROW(t = json);
    CHECK_EQ(t, model::RelationshipType::Realization);
  }
  DOCTEST_TEST_CASE("model::RelationshipType.FromString") {
    CHECK_FALSE(model::RelationshipTypeFromString("invalid").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("aggregation").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("composition").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("inheritance").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("realization").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString(" Aggregation").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString(" Composition").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString(" Inheritance").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString(" Realization").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("Aggregation ").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("Composition ").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("Inheritance ").has_value());
    CHECK_FALSE(model::RelationshipTypeFromString("Realization ").has_value());
    CHECK_EQ(model::RelationshipTypeFromString("Aggregation").value_or(model::RelationshipType::Realization),
             model::RelationshipType::Aggregation);
    CHECK_EQ(model::RelationshipTypeFromString("Composition").value_or(model::RelationshipType::Realization),
             model::RelationshipType::Composition);
    CHECK_EQ(model::RelationshipTypeFromString("Inheritance").value_or(model::RelationshipType::Realization),
             model::RelationshipType::Inheritance);
    CHECK_EQ(model::RelationshipTypeFromString("Realization").value_or(model::RelationshipType::Aggregation),
             model::RelationshipType::Realization);
  }
  DOCTEST_TEST_CASE("model::RelationshipType.Format") {
    CHECK_EQ(std::format("{}", model::RelationshipType::Aggregation), "Aggregation");
    CHECK_EQ(std::format("{}", model::RelationshipType::Composition), "Composition");
    CHECK_EQ(std::format("{}", model::RelationshipType::Inheritance), "Inheritance");
    CHECK_EQ(std::format("{}", model::RelationshipType::Realization), "Realization");
  }
}
