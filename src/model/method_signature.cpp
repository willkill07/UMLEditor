#include "method_signature.hpp"

#include "model/parameter.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <ranges>
#include <utility>

namespace model {

static Result<std::pair<std::vector<std::string>, std::size_t>> ListOfTypes(std::string_view str, std::size_t start) {
  std::vector<std::string> params;
  std::size_t idx{start};
  while (true) {
    if (idx != start) {
      if (idx == str.size()) {
        return std::unexpected{"Unexpected end of type list"};
      } else if (str[idx] != ',') {
        break;
      } else if (++idx == str.size()) {
        return std::unexpected{"Unexpected end of type list after comma"};
      }
    }
    if (auto res = ValidType(str, idx).transform([&](std::size_t i) {
          params.emplace_back(str.substr(idx, i - idx));
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

MethodSignature::MethodSignature(std::string_view name, std::vector<std::string> parameters)
    : name_{name}, parameter_types_{std::move(parameters)} {
}

[[nodiscard]] MethodSignature MethodSignature::WithName(std::string_view name) const {
  MethodSignature copy{*this};
  copy.name_ = name;
  return copy;
}

[[nodiscard]] MethodSignature MethodSignature::WithParameters(std::vector<std::string> parameters) const {
  MethodSignature copy{*this};
  copy.parameter_types_ = std::move(parameters);
  return copy;
}

[[nodiscard]] MethodSignature MethodSignature::WithParameters(std::vector<Parameter> const& parameters) const {
  MethodSignature copy{*this};
  auto rng = std::views::transform(parameters, &Parameter::Type);
  copy.parameter_types_.assign(rng.begin(), rng.end());
  return copy;
}

[[nodiscard]] MethodSignature MethodSignature::WithAddedParameter(std::string_view type) const {
  MethodSignature copy{*this};
  copy.parameter_types_.emplace_back(type);
  return copy;
}

[[nodiscard]] MethodSignature MethodSignature::WithoutParameter(std::size_t index) const {
  MethodSignature copy{*this};
  auto iter = std::next(copy.parameter_types_.begin(), static_cast<long>(index));
  copy.parameter_types_.erase(iter);
  return copy;
}

[[nodiscard]] MethodSignature MethodSignature::WithParameterType(std::size_t index, std::string_view type) const {
  MethodSignature copy{*this};
  copy.parameter_types_[index] = type;
  return copy;
}

[[nodiscard]] std::string const& MethodSignature::Name() const noexcept {
  return name_;
}

[[nodiscard]] std::vector<std::string> const& MethodSignature::ParameterTypes() const noexcept {
  return parameter_types_;
}

Result<MethodSignature> MethodSignature::FromString(std::string_view str) {
  return ValidIdentifier(str).and_then([&](std::size_t idx) -> Result<MethodSignature> {
    auto name = str.substr(0, idx);
    if (idx == str.size() or str[idx] != '(') {
      return std::unexpected{"missing left parenthesis"};
    }
    Result<std::pair<std::vector<std::string>, std::size_t>> list_of_types;
    return ListOfTypes(str, ++idx).and_then([&](auto pair) -> Result<MethodSignature> {
      auto [params, i] = std::move(pair);
      idx = i;
      if (idx == str.size() or str[idx] != ')') {
        return std::unexpected{"missing right parenthesis"};
      }
      if (++idx != str.size()) {
        return std::unexpected{"extra characters at end"};
      }
      return MethodSignature{name, std::move(params)};
    });
  });
}

std::strong_ordering MethodSignature::operator<=>(MethodSignature const& other) const noexcept {
  auto res = (name_ <=> other.name_);
  if (res != 0) {
    return res;
  }
  for (auto [p1, p2] : std::views::zip(parameter_types_, other.parameter_types_)) {
    res = (p1 <=> p2);
    if (res != 0) {
      return res;
    }
  }
  return (parameter_types_.size() <=> other.parameter_types_.size());
}

bool MethodSignature::operator==(MethodSignature const& other) const noexcept {
  return (name_ == other.name_) and std::ranges::equal(parameter_types_, other.parameter_types_);
}

} // namespace model

DOCTEST_TEST_SUITE("model::MethodSignature") {
  DOCTEST_TEST_CASE("model::MethodSignature.Ctor") {
    model::MethodSignature sig{"f", std::vector<std::string>{"p1", "p2"}};
    CHECK_EQ(sig.Name(), "f");
    REQUIRE_EQ(sig.ParameterTypes().size(), 2);
    CHECK_EQ(sig.ParameterTypes()[0], "p1");
    CHECK_EQ(sig.ParameterTypes()[1], "p2");
  }
  DOCTEST_TEST_CASE("model::MethodSignature.FromString") {
    auto s = model::MethodSignature::FromString("f()");
    CHECK(s.has_value());
    CHECK_EQ(s->Name(), "f");
    CHECK(s->ParameterTypes().empty());
    auto s1 = model::MethodSignature::FromString("f(int,str)");
    CHECK(s1.has_value());
    CHECK_EQ(s1->Name(), "f");
    CHECK_EQ(s1->ParameterTypes().size(), 2);
    CHECK_EQ(s1->ParameterTypes()[0], "int");
    CHECK_EQ(s1->ParameterTypes()[1], "str");
    CHECK(model::MethodSignature::FromString("f(int[],int(str))").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f ").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f( ").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f() ").has_value());
    CHECK_FALSE(model::MethodSignature::FromString(" f()").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int, int)").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int,int,)").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int,int,").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int,int").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int,int ").has_value());
    CHECK_FALSE(model::MethodSignature::FromString("f(int[], int(str,))").has_value());
  }
  DOCTEST_TEST_CASE("model::MethodSignature.With") {
    model::MethodSignature const sig{"f", std::vector<std::string>{"p1", "p2"}};
    model::MethodSignature const sig2 = sig.WithName("g");
    CHECK_EQ(sig.Name(), "f");
    CHECK_EQ(sig2.Name(), "g");
    model::MethodSignature const sig3 = sig.WithAddedParameter("p3");
    REQUIRE_EQ(sig3.ParameterTypes().size(), 3);
    CHECK_EQ(sig3.ParameterTypes()[2], "p3");
    model::MethodSignature const sig4 = sig.WithParameterType(0, "p");
    REQUIRE_EQ(sig4.ParameterTypes().size(), 2);
    CHECK_EQ(sig4.ParameterTypes()[0], "p");
    model::MethodSignature const sig5 = sig.WithoutParameter(0);
    REQUIRE_EQ(sig5.ParameterTypes().size(), 1);
    CHECK_EQ(sig5.ParameterTypes()[0], "p2");
    model::MethodSignature const sig6 = sig.WithParameters();
    CHECK(sig6.ParameterTypes().empty());
    model::MethodSignature const sig7 = sig.WithParameters(std::vector<std::string>{"a"});
    REQUIRE_EQ(sig7.ParameterTypes().size(), 1);
    CHECK_EQ(sig7.ParameterTypes()[0], "a");
    model::MethodSignature const sig8 =
        sig.WithParameters(std::vector<model::Parameter>({*model::Parameter::From("a", "int")}));
    REQUIRE_EQ(sig8.ParameterTypes().size(), 1);
    CHECK_EQ(sig8.ParameterTypes()[0], "int");
  }
  DOCTEST_TEST_CASE("model::MethodSignature.Compare") {
    model::MethodSignature const s1{"a", {}}, s2{"a", {"int"}}, s3{"a", {"int", "str"}}, s4{"a", {"str", "str"}},
        s5{"b", {"str", "str"}};
    CHECK_EQ(s1, s1);
    CHECK_EQ(s5, s5);
    CHECK_NE(s1, s5);

    CHECK_LT(s1, s2);
    CHECK_LE(s1, s2);
    CHECK_GT(s2, s1);
    CHECK_GE(s2, s1);

    CHECK_LT(s1, s5);
    CHECK_LE(s1, s5);
    CHECK_GT(s5, s1);
    CHECK_GE(s5, s1);

    CHECK_NE(s1, s3);
    CHECK_NE(s3, s5);
    CHECK_NE(s4, s5);

    CHECK_LT(s3, s4);
    CHECK_LE(s3, s4);
    CHECK_GT(s4, s3);
    CHECK_GE(s4, s3);
  }
  DOCTEST_TEST_CASE("model::MethodSignature.Format") {
    CHECK_EQ(std::format("{}", model::MethodSignature{"f", {"int", "float"}}), "f(int,float)");
  }
}
