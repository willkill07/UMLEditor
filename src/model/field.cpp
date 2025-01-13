#include "field.hpp"

#include <stdexcept>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include "model/checking.hpp"
#include "utils/utils.hpp"

namespace model {

// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann ::json& json, const Field& f) {
  json["name"] = f.Name();
  json["type"] = f.Type();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann ::json& json, Field& f) {
  auto res =
      Field::From(json.at("name").get<std::string>(), json.at("type").get<std::string>()).transform([&](Field field) {
        f = std::move(field);
      });
  if (not res.has_value()) {
    throw std::invalid_argument{res.error()};
  }
}

Result<Field> Field::From(std::string_view name, std::string_view type) {
  return Check<ValidIdentifier>(name, "field name").and_then([&] {
    return Check<ValidType>(type, "field type").transform([&] {
      Field f;
      f.name_ = name;
      f.type_ = type;
      return f;
    });
  });
}

std::string const& Field::Name() const noexcept {
  return name_;
}

std::string const& Field::Type() const noexcept {
  return type_;
}

Result<void> Field::Rename(std::string_view name) {
  return Check<ValidType>(name, "field type").transform([&] { name_ = name; });
}

Result<void> Field::ChangeType(std::string_view new_type) {
  return Check<ValidType>(new_type, "field type").transform([&] { type_ = new_type; });
}

std::strong_ordering Field::operator<=>(Field const& other) const noexcept {
  return name_ <=> other.name_;
}

bool Field::operator==(Field const& other) const noexcept {
  return name_ == other.name_;
}

} // namespace model

DOCTEST_TEST_SUITE("model::Field") {
  DOCTEST_TEST_CASE("model::Field.From") {
    DOCTEST_SUBCASE("Valid") {
      [[maybe_unused]] auto f1 = model::Field::From("valid_name", "valid_type");
      REQUIRE(f1);
      CHECK_EQ(f1->Name(), "valid_name");
      CHECK_EQ(f1->Type(), "valid_type");
    }
    DOCTEST_SUBCASE("Invalid Name") {
      CHECK_FALSE(model::Field::From(" ", "valid_type"));
    }
    DOCTEST_SUBCASE("Invalid Type") {
      CHECK_FALSE(model::Field::From("valid_name", " "));
    }
    DOCTEST_SUBCASE("Invalid Name and Type") {
      CHECK_FALSE(model::Field::From(" ", " "));
    }
  }
  DOCTEST_TEST_CASE("model::Field.Rename") {
    [[maybe_unused]] auto f = model::Field::From("a", "int");
    REQUIRE(f);
    DOCTEST_SUBCASE("Valid") {
      CHECK(f->Rename("b"));
      CHECK_EQ(f->Name(), "b");
    }
    DOCTEST_SUBCASE("Invalid") {
      CHECK_FALSE(f->Rename(" "));
      CHECK_EQ(f->Name(), "a");
    }
  }
  DOCTEST_TEST_CASE("model::Field.ChangeType") {
    [[maybe_unused]] auto f = model::Field::From("a", "int");
    REQUIRE(f);
    DOCTEST_SUBCASE("Valid") {
      CHECK(f->ChangeType("double"));
      CHECK_EQ(f->Type(), "double");
    }
    DOCTEST_SUBCASE("Invalid") {
      CHECK_FALSE(f->ChangeType(" "));
      CHECK_EQ(f->Type(), "int");
    }
  }
  DOCTEST_TEST_CASE("model::Field.Json") {
    DOCTEST_SUBCASE("Serialize") {
      DOCTEST_SUBCASE("Valid") {
        [[maybe_unused]] auto res = model::Field::From("a", "int").transform([&]([[maybe_unused]] auto&& f) {
          [[maybe_unused]] nlohmann::json j;
          REQUIRE_NOTHROW(j = f);
          CHECK_EQ(f.Name(), j["name"].get<std::string>());
          CHECK_EQ(f.Type(), j["type"].get<std::string>());
        });
        REQUIRE(res);
      }
    }
    DOCTEST_SUBCASE("Deserialize") {
      nlohmann::json j;
      [[maybe_unused]] model::Field f;
      DOCTEST_SUBCASE("Valid") {
        j["name"] = "a";
        j["type"] = "int";
        REQUIRE_NOTHROW(f = j);
        CHECK_EQ(f.Name(), "a");
        CHECK_EQ(f.Type(), "int");
      }
      DOCTEST_SUBCASE("Invalid Name") {
        j["name"] = " ";
        j["type"] = "int";
        CHECK_THROWS(f = j);
      }
      DOCTEST_SUBCASE("Invalid Type") {
        j["name"] = "a";
        j["type"] = " ";
        CHECK_THROWS(f = j);
      }
      DOCTEST_SUBCASE("Invalid Name and Type") {
        j["name"] = " ";
        j["type"] = " ";
        CHECK_THROWS(f = j);
      }
    }
  }
  DOCTEST_TEST_CASE("model::Field.Comparison") {
    [[maybe_unused]] auto a = model::Field::From("a", "int");
    [[maybe_unused]] auto b = model::Field::From("b", "int");
    [[maybe_unused]] auto aa = model::Field::From("a", "int");
    CHECK_NE(*a, *b);
    CHECK_LT(*a, *b);
    CHECK_LE(*a, *b);
    CHECK_GT(*b, *a);
    CHECK_GE(*b, *a);
    CHECK_EQ(*a, *aa);
    CHECK_EQ(*a, *a);
    CHECK_LE(*a, *a);
    CHECK_GE(*a, *a);
  }

  DOCTEST_TEST_CASE("model::Field.Format") {
    [[maybe_unused]] auto a = model::Field::From("a", "int");
    REQUIRE(a);
    CHECK_EQ(std::format("{}", *a), "a:int");
    CHECK_EQ(std::format("{: }", *a), "a: int");
  }
}
