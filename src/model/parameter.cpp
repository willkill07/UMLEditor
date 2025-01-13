#include "parameter.hpp"

#include "model/checking.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace model {

// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann ::json& json, const Parameter& p) {
  json["name"] = p.Name();
  json["type"] = p.Type();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann ::json& json, Parameter& p) {
  auto res = Parameter::From(json.at("name").get<std::string>(), json.at("type").get<std::string>())
                 .transform([&](Parameter param) { p = std::move(param); });
  if (not res) {
    throw std::invalid_argument{res.error()};
  }
}

Result<Parameter> Parameter::From(std::string_view name, std::string_view type) {
  return Check<ValidIdentifier>(name, "parameter name").and_then([&] {
    return Check<ValidType>(type, "parameter type").transform([&] {
      Parameter param;
      param.name_ = name;
      param.type_ = type;
      return param;
    });
  });
}

std::string const& Parameter::Name() const noexcept {
  return name_;
}

std::string const& Parameter::Type() const noexcept {
  return type_;
}

Result<void> Parameter::Rename(std::string_view new_name) {
  return Check<ValidIdentifier>(new_name, "parameter name").transform([&] { name_ = new_name; });
}

Result<void> Parameter::ChangeType(std::string_view new_type) {
  return Check<ValidType>(new_type, "parameter type").transform([&] { type_ = new_type; });
}

Result<std::pair<Parameter, std::size_t>> Parameter::Parse(std::string_view str, std::size_t start) {
  return ValidIdentifier(str, start)
      .and_then([&](std::size_t colon_index) -> Result<std::pair<Parameter, std::size_t>> {
        auto name = str.substr(start, colon_index - start);
        if (colon_index == str.size() or str[colon_index] != ':') {
          return std::unexpected{std::format("missing colon at index {}", colon_index)};
        }
        return ValidType(str, ++colon_index)
            .and_then([&](std::size_t end_index) -> Result<std::pair<Parameter, std::size_t>> {
              auto type = str.substr(colon_index, end_index - colon_index);
              return Parameter::From(name, type).transform([&](auto p) { return std::pair{std::move(p), end_index}; });
            });
      });
}

Result<Parameter> Parameter::FromString(std::string_view str) {
  return Parse(str).and_then([&](auto&& pair) -> Result<Parameter> {
    if (auto [param, end] = pair; end != str.size()) {
      return std::unexpected{std::format("extra characters encountered: {}", str.substr(end))};
    } else {
      return param;
    }
  });
}

Result<std::pair<std::vector<Parameter>, std::size_t>> Parameter::ParseMultiple(std::string_view str,
                                                                                std::size_t start) {
  std::vector<Parameter> params;
  std::size_t idx{start};
  while (idx != str.size()) {
    if (idx != start) {
      if (str[idx] != ',') {
        break;
      } else {
        ++idx;
      }
    }
    if (auto res = Parse(str, idx).transform([&](auto pair) {
          auto [p, i] = std::move(pair);
          params.push_back(std::move(p));
          idx = i;
        });
        not res) {
      if (idx == start) {
        break;
      }
      return std::unexpected{std::move(res.error())};
    }
  }
  return std::pair{std::move(params), idx};
}

Result<std::vector<Parameter>> Parameter::MultipleFromString(std::string_view str) {
  return ParseMultiple(str).and_then([&](auto&& pair) -> Result<std::vector<Parameter>> {
    if (auto [params, end] = pair; end != str.size()) {
      return std::unexpected{std::format("extra characters encountered: {}", str.substr(end))};
    } else {
      return params;
    }
  });
}

std::strong_ordering Parameter::operator<=>(Parameter const& other) const noexcept {
  return name_ <=> other.name_;
}

bool Parameter::operator==(Parameter const& other) const noexcept {
  return name_ == other.name_;
}

} // namespace model

DOCTEST_TEST_SUITE("model::Parameter") {
  DOCTEST_TEST_CASE("model::Parameter.From") {
    DOCTEST_SUBCASE("Valid") {
      auto f1 = model::Parameter::From("valid_name", "valid_type");
      REQUIRE(f1.has_value());
      CHECK_EQ(f1->Name(), "valid_name");
      CHECK_EQ(f1->Type(), "valid_type");
    }
    DOCTEST_SUBCASE("Invalid Name") {
      CHECK(not model::Parameter::From(" ", "valid_type").has_value());
    }
    DOCTEST_SUBCASE("Invalid Type") {
      CHECK(not model::Parameter::From("valid_name", " ").has_value());
    }
    DOCTEST_SUBCASE("Invalid Name and Type") {
      CHECK(not model::Parameter::From(" ", " ").has_value());
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.FromString") {
    DOCTEST_SUBCASE("Valid") {
      auto p = model::Parameter::FromString("name:type");
      REQUIRE(p.has_value());
      CHECK_EQ(p->Name(), "name");
      CHECK_EQ(p->Type(), "type");
    }
    DOCTEST_SUBCASE("Invalid") {
      CHECK_FALSE(model::Parameter::FromString("name:type ").has_value());
      CHECK_FALSE(model::Parameter::FromString("name: type").has_value());
      CHECK_FALSE(model::Parameter::FromString("name :type").has_value());
      CHECK_FALSE(model::Parameter::FromString(" name:type").has_value());
      CHECK_FALSE(model::Parameter::FromString("name->type").has_value());
      CHECK_FALSE(model::Parameter::FromString("name,type").has_value());
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.MultipleFromString") {
    DOCTEST_SUBCASE("Valid") {
      DOCTEST_SUBCASE("Empty") {
        auto p = model::Parameter::MultipleFromString("");
        REQUIRE(p.has_value());
        CHECK(p->empty());
      }
      DOCTEST_SUBCASE("One") {
        auto p = model::Parameter::MultipleFromString("a:int");
        REQUIRE(p.has_value());
        CHECK_EQ(p->size(), 1);
        CHECK_EQ(p->front().Name(), "a");
        CHECK_EQ(p->front().Type(), "int");
      }
      DOCTEST_SUBCASE("Many") {
        auto p = model::Parameter::MultipleFromString("a:int,b:str,c:number");
        REQUIRE(p.has_value());
        CHECK_EQ(p->size(), 3);
        CHECK_EQ(p->at(0).Name(), "a");
        CHECK_EQ(p->at(0).Type(), "int");
        CHECK_EQ(p->at(1).Name(), "b");
        CHECK_EQ(p->at(1).Type(), "str");
        CHECK_EQ(p->at(2).Name(), "c");
        CHECK_EQ(p->at(2).Type(), "number");
      }
    }
    DOCTEST_SUBCASE("Invalid") {
      DOCTEST_SUBCASE("Whitespace") {
        CHECK_FALSE(model::Parameter::MultipleFromString(" ").has_value());
      }
      DOCTEST_SUBCASE("One") {
        CHECK_FALSE(model::Parameter::FromString("name:type ").has_value());
        CHECK_FALSE(model::Parameter::FromString("name: type").has_value());
        CHECK_FALSE(model::Parameter::FromString("name :type").has_value());
        CHECK_FALSE(model::Parameter::FromString(" name:type").has_value());
        CHECK_FALSE(model::Parameter::FromString("name->type").has_value());
        CHECK_FALSE(model::Parameter::FromString("name,type").has_value());
      }
      DOCTEST_SUBCASE("Many") {
        CHECK_FALSE(model::Parameter::MultipleFromString(" a:int,b:str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a :int,b:str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a: int,b:str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int ,b:str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int, b:str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b :str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b: str,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b:str ,c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b:str, c:number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b:str,c :number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b:str,c: number").has_value());
        CHECK_FALSE(model::Parameter::MultipleFromString("a:int,b:str,c:number,").has_value());
      }
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.Rename") {
    DOCTEST_SUBCASE("Valid") {
      auto f = model::Parameter::From("a", "int");
      CHECK(f->Rename("b").has_value());
      CHECK_EQ(f->Name(), "b");
    }
    DOCTEST_SUBCASE("Invalid") {
      auto f = model::Parameter::From("a", "int");
      CHECK(not f->Rename(" ").has_value());
      CHECK_EQ(f->Name(), "a");
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.ChangeType") {
    DOCTEST_SUBCASE("Valid") {
      auto f = model::Parameter::From("a", "int");
      CHECK(f->ChangeType("double").has_value());
      CHECK_EQ(f->Type(), "double");
    }
    DOCTEST_SUBCASE("Invalid") {
      auto f = model::Parameter::From("a", "int");
      CHECK(not f->ChangeType(" ").has_value());
      CHECK_EQ(f->Type(), "int");
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.Json") {
    DOCTEST_SUBCASE("Serialize") {
      DOCTEST_SUBCASE("Valid") {
        auto res = model::Parameter::From("a", "int").transform([&](auto&& f) {
          nlohmann::json j;
          j = f;
          CHECK_EQ(j["name"], f.Name());
          CHECK_EQ(j["type"], f.Type());
        });
        REQUIRE(res.has_value());
      }
    }
    DOCTEST_SUBCASE("Deserialize") {
      DOCTEST_SUBCASE("Valid") {
        nlohmann::json j;
        j["name"] = "a";
        j["type"] = "int";
        model::Parameter f = j;
        CHECK_EQ(f.Name(), "a");
        CHECK_EQ(f.Type(), "int");
      }
      DOCTEST_SUBCASE("Invalid Name") {
        nlohmann::json j;
        j["name"] = " ";
        j["type"] = "int";
        model::Parameter f;
        CHECK_THROWS(f = j);
      }
      DOCTEST_SUBCASE("Invalid Type") {
        nlohmann::json j;
        j["name"] = "a";
        j["type"] = " ";
        model::Parameter f;
        CHECK_THROWS(f = j);
      }
      DOCTEST_SUBCASE("Invalid Name and Type") {
        nlohmann::json j;
        j["name"] = " ";
        j["type"] = " ";
        model::Parameter f;
        CHECK_THROWS(f = j);
      }
    }
  }
  DOCTEST_TEST_CASE("model::Parameter.Comparison") {
    auto a = model::Parameter::From("a", "int");
    auto b = model::Parameter::From("b", "int");
    auto aa = model::Parameter::From("a", "int");
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

  DOCTEST_TEST_CASE("model::Parameter.Format") {
    auto a = model::Parameter::From("a", "int");
    REQUIRE(a.has_value());
    CHECK_EQ(std::format("{}", *a), "a:int");
    CHECK_EQ(std::format("{: }", *a), "a: int");
  }
}
