#include "method.hpp"

#include "model/checking.hpp"
#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>

static Result<void> Check(std::vector<model::Parameter> const& parameters) {
  auto param_names = std::ranges::to<std::vector>(std::views::transform(parameters, &model::Parameter::Name));
  return Unique(param_names, "parameter names")
      .and_then([&] { return CheckAll<ValidIdentifier>(param_names, "parameter name"); })
      .and_then([&] {
        return CheckAll<ValidType>(std::views::transform(parameters, &model::Parameter::Type), "parameter type");
      });
}

namespace model {

// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann ::json& json, const Method& m) {
  json["name"] = m.Name();
  json["return_type"] = m.ReturnType();
  json["params"] = m.Parameters();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(const nlohmann ::json& json, Method& m) {
  auto res = Method::From(json.at("name").get<std::string>(),
                          json.at("return_type").get<std::string>(),
                          json.at("params").get<std::vector<Parameter>>())
                 .transform([&](Method method) { m = std::move(method); });
  if (not res) {
    throw std::invalid_argument{res.error()};
  }
}

Result<Method> Method::From(std::string_view name, std::string_view return_type, std::vector<Parameter> parameters) {
  return Check<ValidIdentifier>(name, "method name")
      .and_then([&] { return Check<ValidType>(return_type, "method return type"); })
      .and_then([&] { return Check(parameters); })
      .transform([&] {
        Method m;
        m.name_ = name;
        m.return_type_ = return_type;
        m.parameters_ = std::move(parameters);
        return m;
      });
}

std::string const& Method::Name() const noexcept {
  return name_;
}
std::string const& Method::ReturnType() const noexcept {
  return return_type_;
}
std::vector<Parameter> const& Method::Parameters() const noexcept {
  return parameters_;
}

Result<void> Method::AddParameter(std::string_view parameter_name, std::string_view parameter_type) {
  return Parameter::From(parameter_name, parameter_type).and_then([&](Parameter p) -> Result<void> {
    if (std::ranges::find(parameters_, p) != parameters_.end()) {
      return std::unexpected{"adding duplicate parameter"};
    } else {
      parameters_.push_back(std::move(p));
      return {};
    }
  });
}

Result<void> Method::ClearParameters() {
  parameters_.clear();
  return {};
}

Result<void> Method::RemoveParameter(std::vector<Parameter>::const_iterator iter) {
  parameters_.erase(iter);
  return {};
}

Result<void> Method::Rename(std::string_view name) {
  return Check<ValidIdentifier>(name, "method name").transform([&] { name_ = name; });
}

Result<void> Method::RenameParameter(std::string_view parameter_name, std::string_view new_name) {
  return GetParameter(parameter_name).and_then([&](auto p) -> Result<void> {
    if (std::ranges::contains(parameters_, new_name, &Parameter::Name)) {
      return std::unexpected{"duplicate parameter name"};
    } else {
      return p->Rename(new_name);
    }
  });
}

Result<void> Method::ChangeReturnType(std::string_view new_type) {
  return Check<ValidType>(new_type, "method return type").transform([&] { return_type_ = new_type; });
}

Result<std::vector<Parameter>::iterator> Method::GetParameter(std::string_view parameter_name) {
  if (auto i = std::ranges::find(parameters_, parameter_name, &Parameter::Name); i != parameters_.end()) {
    return i;
  } else {
    return std::unexpected{std::format("method parameter '{}' does not exist", parameter_name)};
  }
}

Result<std::vector<Parameter>::const_iterator> Method::GetReadOnlyParameter(std::string_view parameter_name) const {
  if (auto i = std::ranges::find(parameters_, parameter_name, &Parameter::Name); i != parameters_.end()) {
    return i;
  } else {
    return std::unexpected{std::format("method parameter '{}' does not exist", parameter_name)};
  }
}

std::size_t Method::GetParameterIndex(std::vector<Parameter>::const_iterator i) const {
  return static_cast<std::size_t>(std::distance(parameters_.begin(), i));
}

[[nodiscard]] Result<void> Method::ChangeParameters(std::vector<Parameter> parameters) {
  return Check(parameters).transform([&] { parameters_ = std::move(parameters); });
}

Result<Method> Method::FromString(std::string_view str) {
  return ValidIdentifier(str).and_then([&](std::size_t idx) -> Result<Method> {
    auto name = str.substr(0, idx);
    if (idx == str.size() or str[idx] != '(') {
      return std::unexpected{"missing left parenthesis"};
    }
    return Parameter::ParseMultiple(str, ++idx).and_then([&](auto pair) -> Result<Method> {
      auto [params, i] = std::move(pair);
      idx = i;
      if (idx == str.size() or str[idx] != ')') {
        return std::unexpected{"missing right parenthesis"};
      }
      ++idx;
      if (idx == str.size() or str[idx] != '-') {
        return std::unexpected{"missing arrow"};
      }
      ++idx;
      if (idx == str.size() or str[idx] != '>') {
        return std::unexpected{"missing arrow"};
      }
      return ValidType(str, ++idx).and_then([&](std::size_t j) -> Result<Method> {
        auto return_type = str.substr(idx, j);
        if (j != str.size()) {
          return std::unexpected{std::format("extra characters encountered: {}", str.substr(j))};
        }
        return Method::From(name, return_type, std::move(params));
      });
    });
  });
}

std::string Method::ToSignatureString() const {
  MethodSignature sig(name_, std::ranges::to<std::vector>(std::views::transform(parameters_, &Parameter::Type)));
  return std::format("{}", sig);
}

std::strong_ordering Method::operator<=>(Method const& other) const noexcept {
  auto res = (name_ <=> other.name_);
  if (res != 0) {
    return res;
  }
  res = (parameters_.size() <=> other.parameters_.size());
  if (res != 0) {
    return res;
  }
  for (auto [p1, p2] : std::views::zip(parameters_, other.parameters_)) {
    res = (p1.Type() <=> p2.Type());
    if (res != 0) {
      break;
    }
  }
  if (res != 0) {
    return res;
  }
  return return_type_ <=> other.return_type_;
}

bool Method::operator==(Method const& other) const noexcept {
  return (name_ == other.name_) and
         std::ranges::equal(parameters_, other.parameters_, {}, &Parameter::Type, &Parameter::Type);
}

bool Method::operator==(MethodSignature const& sig) const noexcept {
  return (name_ == sig.Name()) and std::ranges::equal(parameters_, sig.ParameterTypes(), {}, &Parameter::Type);
}

} // namespace model

DOCTEST_TEST_SUITE("model::Method") {
  DOCTEST_TEST_CASE("model::Method.From") {
    auto f = model::Method::From("f", "void", *model::Parameter::MultipleFromString("a:int,b:str"));
    REQUIRE(f.has_value());
    CHECK_EQ(f->Name(), "f");
    CHECK_EQ(f->ReturnType(), "void");
    REQUIRE_EQ(f->Parameters().size(), 2);
    CHECK_EQ(f->Parameters()[0].Name(), "a");
    CHECK_EQ(f->Parameters()[0].Type(), "int");
    CHECK_EQ(f->Parameters()[1].Name(), "b");
    CHECK_EQ(f->Parameters()[1].Type(), "str");
    CHECK_FALSE(model::Method::From(" invalid", "void", {}).has_value());
    CHECK_FALSE(model::Method::From("f", " invalid", {}).has_value());
    CHECK_FALSE(model::Method::From("f", "void", *model::Parameter::MultipleFromString("a:int,a:str")).has_value());
  }
  DOCTEST_TEST_CASE("model::Method.FromString") {
    auto m = model::Method::FromString("f()->void");
    REQUIRE(m.has_value());
    CHECK_EQ(m->Name(), "f");
    CHECK_EQ(m->ReturnType(), "void");
    CHECK(m->Parameters().empty());
    m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK_EQ(m->Name(), "f");
    CHECK_EQ(m->ReturnType(), "void");
    CHECK_EQ(m->Parameters().size(), 2);
    CHECK_EQ(m->Parameters()[0].Name(), "a");
    CHECK_EQ(m->Parameters()[0].Type(), "int");
    CHECK_EQ(m->Parameters()[1].Name(), "b");
    CHECK_EQ(m->Parameters()[1].Type(), "str");
    CHECK_FALSE(model::Method::FromString("f(a:int,a:str)->void"));
    CHECK_FALSE(model::Method::FromString(" f()->void").has_value());
    CHECK_FALSE(model::Method::FromString("f ()->void").has_value());
    CHECK_FALSE(model::Method::FromString("f( )->void").has_value());
    CHECK_FALSE(model::Method::FromString("f() ->void").has_value());
    CHECK_FALSE(model::Method::FromString("f()- >void").has_value());
    CHECK_FALSE(model::Method::FromString("f()-> void").has_value());
    CHECK_FALSE(model::Method::FromString("f()->void ").has_value());
    CHECK_FALSE(model::Method::FromString("f ").has_value());
    CHECK_FALSE(model::Method::FromString("f(").has_value());
    CHECK_FALSE(model::Method::FromString("f()").has_value());
    CHECK_FALSE(model::Method::FromString("f()-").has_value());
    CHECK_FALSE(model::Method::FromString("f()->").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:)->void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,)->void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b)->void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:)->void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int) ->void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int) ").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int)-").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int)- ").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int)-> void").has_value());
    CHECK_FALSE(model::Method::FromString("f(a:int,b:int)->").has_value());
    CHECK_FALSE(model::Method::FromString("f()- >void").has_value());
    CHECK_FALSE(model::Method::FromString("f()-> void").has_value());
    CHECK_FALSE(model::Method::FromString("f()->void ").has_value());
    CHECK_FALSE(model::Method::FromString("f ").has_value());
    CHECK_FALSE(model::Method::FromString("f(").has_value());
    CHECK_FALSE(model::Method::FromString("f()").has_value());
    CHECK_FALSE(model::Method::FromString("f()-").has_value());
    CHECK_FALSE(model::Method::FromString("f()->").has_value());
  }
  DOCTEST_TEST_CASE("model::Method.Json") {
    nlohmann::json json;
    model::Method m;
    REQUIRE_NOTHROW(m = R"( { "name": "f", "return_type": "void", "params":[] } )"_json);
    CHECK_EQ(m.Name(), "f");
    CHECK_EQ(m.ReturnType(), "void");
    CHECK(m.Parameters().empty());
    REQUIRE_NOTHROW(m = R"(
        {
          "name": "g",
          "return_type": "int",
          "params": [
            {"name": "a", "type": "int"},
            {"name": "b", "type": "str"}
          ]
        }
        )"_json);
    CHECK_EQ(m.Name(), "g");
    CHECK_EQ(m.ReturnType(), "int");
    REQUIRE_EQ(m.Parameters().size(), 2);
    CHECK_EQ(m.Parameters()[0].Name(), "a");
    CHECK_EQ(m.Parameters()[0].Type(), "int");
    CHECK_EQ(m.Parameters()[1].Name(), "b");
    CHECK_EQ(m.Parameters()[1].Type(), "str");
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"int","params":[{"name":"a","type":"int"},{"name":"a","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"","return_type":"int","params":[{"name":"a","type":"int"},{"name":"b","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"","params":[{"name":"a","type":"int"},{"name":"b","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"int","params":[{"name":"","type":"int"},{"name":"b","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"int","params":[{"name":"a","type":""},{"name":"b","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"int","params":[{"name":"a","type":"int"},{"name":"","type":"str"}]})"_json);
    REQUIRE_THROWS(
        m = R"({"name":"g","return_type":"int","params":[{"name":"a","type":"int"},{"name":"b","type":""}]})"_json);
    nlohmann::json j =
        R"({"name":"g","return_type":"int","params":[{"name":"a","type":"int"},{"name":"b","type":"str"}]})"_json;
    REQUIRE_NOTHROW(json = m = j);
    CHECK_EQ(json, j);
  }
  DOCTEST_TEST_CASE("model::Method.ToSignatureString") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK_EQ(m->ToSignatureString(), "f(int,str)");
    m = model::Method::FromString("f()->void");
    REQUIRE(m.has_value());
    CHECK_EQ(m->ToSignatureString(), "f()");
  }
  DOCTEST_TEST_CASE("model::Method.Rename") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK(m->Rename("x"));
    CHECK_EQ(m->Name(), "x");
    CHECK_FALSE(m->Rename(" "));
  }
  DOCTEST_TEST_CASE("model::Method.ChangeReturnType") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK(m->ChangeReturnType("x"));
    CHECK_EQ(m->ReturnType(), "x");
    CHECK_FALSE(m->ChangeReturnType(" "));
  }
  DOCTEST_TEST_CASE("model::Method.AddParameter") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    REQUIRE(m->AddParameter("name", "type"));
    REQUIRE_EQ(m->Parameters().size(), 3);
    CHECK_EQ(m->Parameters().back().Name(), "name");
    CHECK_EQ(m->Parameters().back().Type(), "type");
    CHECK_FALSE(m->AddParameter("a", "number"));
    CHECK_FALSE(m->AddParameter("b", "number"));
    CHECK_FALSE(m->AddParameter("c", " "));
    CHECK_FALSE(m->AddParameter(" ", "int"));
  }
  DOCTEST_TEST_CASE("model::Method.RemoveParameter") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    auto p_iter = m->GetReadOnlyParameter("a");
    REQUIRE(p_iter.has_value());
    REQUIRE(m->RemoveParameter(*p_iter));
    CHECK_EQ(m->Parameters().size(), 1);
    CHECK_FALSE(m->GetReadOnlyParameter("a"));
    p_iter = m->GetReadOnlyParameter("b");
    REQUIRE(p_iter.has_value());
    REQUIRE(m->RemoveParameter(*p_iter));
    CHECK(m->Parameters().empty());
  }
  DOCTEST_TEST_CASE("model::Method.RenameParameter") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK_FALSE(m->RenameParameter("a", "b"));
    CHECK_FALSE(m->RenameParameter("b", "a"));
    CHECK_FALSE(m->RenameParameter(" ", "c"));
    CHECK_FALSE(m->RenameParameter("a", " "));
    CHECK(m->RenameParameter("a", "c"));
    CHECK_EQ(m->Parameters()[0].Name(), "c");
    CHECK_EQ(m->Parameters()[0].Type(), "int");
    CHECK_EQ(m->Parameters()[1].Name(), "b");
    CHECK_EQ(m->Parameters()[1].Type(), "str");
  }
  DOCTEST_TEST_CASE("model::Method.ClearParameters") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK(m->ClearParameters().has_value());
    CHECK(m->Parameters().empty());
  }
  DOCTEST_TEST_CASE("model::Method.ChangeParameters") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK(m->ChangeParameters(*model::Parameter::MultipleFromString("d:any")));
    REQUIRE_EQ(m->Parameters().size(), 1);
    CHECK_EQ(m->Parameters()[0].Name(), "d");
    CHECK_EQ(m->Parameters()[0].Type(), "any");
    CHECK_FALSE(m->ChangeParameters(*model::Parameter::MultipleFromString("d:any,d:int")));
  }
  DOCTEST_TEST_CASE("model::Method.GetParameter") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    auto i = m->GetReadOnlyParameter("a");
    REQUIRE(i);
    CHECK_EQ((*i)->Name(), "a");
    i = m->GetReadOnlyParameter("b");
    REQUIRE(i);
    CHECK_EQ((*i)->Name(), "b");
    CHECK_FALSE(m->GetReadOnlyParameter("c"));
    auto j = m->GetParameter("a");
    REQUIRE(j);
    CHECK_EQ((*j)->Name(), "a");
    j = m->GetParameter("b");
    REQUIRE(j);
    CHECK_EQ((*j)->Name(), "b");
    CHECK_FALSE(m->GetParameter("c"));
  }
  DOCTEST_TEST_CASE("model::Method.GetParameterIndex") {
    auto m = model::Method::FromString("f(a:int,b:str,c:any)->void");
    REQUIRE(m.has_value());
    auto i = m->GetReadOnlyParameter("a");
    REQUIRE(i);
    CHECK_EQ(m->GetParameterIndex(*i), 0);
    i = m->GetReadOnlyParameter("b");
    REQUIRE(i);
    CHECK_EQ(m->GetParameterIndex(*i), 1);
    i = m->GetReadOnlyParameter("c");
    REQUIRE(i);
    CHECK_EQ(m->GetParameterIndex(*i), 2);
  }
  DOCTEST_TEST_CASE("model::Method.Compare") {
    std::vector m{model::Method::FromString("e()->void"),
                  model::Method::FromString("f(z:float)->void"),
                  model::Method::FromString("f(x:int)->void"),
                  model::Method::FromString("f(y:float,b:int)->void"),
                  model::Method::FromString("f(w:int,b:int)->void")};
    auto n = model::Method::FromString("f(w:int,b:int)->void");
    REQUIRE(n);
    model::MethodSignature const ms{"f", {"int", "int"}}, ms1{"f", {"int"}}, ms2{"f", {"int", "float"}};
    CHECK(*n == ms);
    CHECK(*n != ms1);
    CHECK(*n != ms2);

    REQUIRE(m.back());
    CHECK_EQ(*m.back(), *n);
    CHECK_GE(*m.back(), *n);
    CHECK_LE(*m.back(), *n);
    for (std::size_t i{0}; i < m.size(); ++i) {
      REQUIRE(m[i]);
      CHECK_EQ(*m[i], *m[i]);
      CHECK_LE(*m[i], *m[i]);
      CHECK_GE(*m[i], *m[i]);
      for (std::size_t j{i + 1}; j < m.size(); ++j) {
        REQUIRE(m[j]);
        CHECK_LT(*m[i], *m[j]);
        CHECK_LE(*m[i], *m[j]);
        CHECK_NE(*m[i], *m[j]);
        CHECK_GT(*m[j], *m[i]);
        CHECK_GE(*m[j], *m[i]);
      }
    }
  }
  DOCTEST_TEST_CASE("model::Method.Format") {
    auto m = model::Method::FromString("f(a:int,b:str)->void");
    REQUIRE(m.has_value());
    CHECK_EQ(std::format("{}", *m), "f(a:int,b:str)->void");
    CHECK_EQ(std::format("{: }", *m), "f(a: int, b: str) -> void");
  }
}
