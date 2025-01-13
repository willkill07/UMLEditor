#include "class.hpp"

#include "model/checking.hpp"
#include "model/method.hpp"
#include "model/method_signature.hpp"
#include "model/parameter.hpp"
#include "nlohmann/json_fwd.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>
#include <stdexcept>

static Result<void> Check(std::vector<model::Parameter> const& parameters) {
  return CheckAll<ValidIdentifier>(std::views::transform(parameters, &model::Parameter::Name), "parameter name")
      .and_then([&] {
        return CheckAll<ValidType>(std::views::transform(parameters, &model::Parameter::Type), "parameter type");
      })
      .and_then([&] {
        auto names = std::ranges::to<std::vector>(std::views::transform(parameters, &model::Parameter::Name));
        return Unique(names, "parameter names");
      });
}

static Result<void> Check(model::MethodSignature const& sig) {
  return Check<ValidIdentifier>(sig.Name(), "method name").and_then([&] {
    return CheckAll<ValidType>(sig.ParameterTypes(), "parameter type");
  });
}

static Result<void> Check(model::Method const& method) {
  return Check<ValidIdentifier>(method.Name(), "method name")
      .and_then([&] { return Check(method.Parameters()); })
      .and_then([&] { return Check<ValidType>(method.ReturnType(), "return type"); })
      .and_then([&] {
        auto names = std::ranges::to<std::vector>(std::views::transform(method.Parameters(), &model::Parameter::Name));
        return Unique(names, "parameter names");
      });
}

namespace model {

/// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann::json& json, Point const& point) {
  json["x"] = point.x;
  json["y"] = point.y;
}

/// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(nlohmann::json const& json, Point& point) {
  json.at("x").get_to(point.x);
  json.at("y").get_to(point.y);
}

/// NOLINTNEXTLINE(readability-identifier-naming)
void to_json(nlohmann::json& json, Class const& c) {
  json["name"] = c.name_;
  json["fields"] = c.fields_;
  json["methods"] = c.methods_;
  json["position"] = c.position_;
}

/// NOLINTNEXTLINE(readability-identifier-naming)
void from_json(nlohmann::json const& json, Class& c) {
  json.at("name").get_to(c.name_);
  if (auto res = Check<ValidType>(c.name_, "class name"); not res) {
    throw std::invalid_argument{res.error()};
  } else {
    json.at("fields").get_to(c.fields_);
    json.at("methods").get_to(c.methods_);
    json.at("position").get_to(c.position_);
  }
}

Result<Class> Class::From(std::string_view name) {
  return Check<ValidType>(name, "class name").transform([&] {
    Class c;
    c.name_ = name;
    return c;
  });
}

std::string const& Class::Name() const noexcept {
  return name_;
}

std::vector<Field> const& Class::Fields() const noexcept {
  return fields_;
}

std::vector<Method> const& Class::Methods() const noexcept {
  return methods_;
}

Point const& Class::Position() const noexcept {
  return position_;
}

Result<void> Class::Rename(std::string_view name) {
  return Check<ValidType>(name, "class name").transform([&] { name_ = name; });
}

Result<std::vector<Field>::iterator> Class::GetField(std::string_view field_name) {
  return Check<ValidIdentifier>(field_name, "field name").and_then([&]() -> Result<std::vector<Field>::iterator> {
    if (auto i = std::ranges::find(fields_, field_name, &Field::Name); i != fields_.end()) {
      return i;
    } else {
      return std::unexpected{std::format("field '{}' does not exist", field_name)};
    };
  });
}

Result<std::vector<Field>::const_iterator> Class::GetReadOnlyField(std::string_view field_name) const {
  return Check<ValidIdentifier>(field_name, "field name").and_then([&]() -> Result<std::vector<Field>::const_iterator> {
    if (auto i = std::ranges::find(fields_, field_name, &Field::Name); i != fields_.end()) {
      return i;
    } else {
      return std::unexpected{std::format("field '{}' does not exist", field_name)};
    };
  });
}

Result<std::vector<Method>::iterator> Class::GetMethodFromSignature(MethodSignature const& method_signature) {
  return Check(method_signature).and_then([&]() -> Result<std::vector<Method>::iterator> {
    if (auto i = std::ranges::find_if(methods_, [&](Method const& m) { return method_signature == m; });
        i != methods_.end()) {
      return i;
    } else {
      return std::unexpected{"method does not exist"};
    }
  });
}

Result<std::vector<Method>::const_iterator>
Class::GetReadOnlyMethodFromSignature(MethodSignature const& method_signature) const {
  return Check(method_signature).and_then([&]() -> Result<std::vector<Method>::const_iterator> {
    if (auto i = std::ranges::find_if(methods_, [&](Method const& m) { return method_signature == m; });
        i != methods_.end()) {
      return i;
    } else {
      return std::unexpected{"method does not exist"};
    }
  });
}

Result<std::vector<Method>::iterator> Class::GetMethod(Method const& method) {
  return Check(method).and_then([&]() -> Result<std::vector<Method>::iterator> {
    if (auto i = std::ranges::find(methods_, method); i != methods_.end()) {
      return i;
    } else {
      return std::unexpected{"method does not exist"};
    }
  });
}

Result<std::vector<Method>::const_iterator> Class::GetReadOnlyMethod(Method const& method) const {
  return Check(method).and_then([&]() -> Result<std::vector<Method>::const_iterator> {
    if (auto i = std::ranges::find(methods_, method); i != methods_.end()) {
      return i;
    } else {
      return std::unexpected{"method does not exist"};
    }
  });
}

Result<void> Class::AddField(std::string_view field_name, std::string_view field_type) {
  if (not GetField(field_name)) {
    return Field::From(field_name, field_type).transform([&](Field f) {
      fields_.push_back(std::move(f));
      std::ranges::sort(fields_);
    });
  } else {
    return std::unexpected{"the new field already exists"};
  }
}

Result<void> Class::DeleteField(std::string_view field_name) {
  return GetField(field_name).transform([&](auto f) { fields_.erase(f); });
}

Result<void> Class::RenameField(std::string_view field_name, std::string_view new_name) {
  return GetField(field_name).and_then([&](auto f) -> Result<void> {
    if (not GetField(new_name)) {
      return f->Rename(new_name).transform([&] { std::ranges::sort(fields_); });
    } else {
      return std::unexpected{"the new field name is already in use"};
    }
  });
}

Result<void> Class::AddMethod(std::string_view name, std::string_view return_type, std::vector<Parameter> parameters) {
  return Method::From(name, return_type, std::move(parameters)).and_then([&](Method method) -> Result<void> {
    if (not GetMethod(method)) {
      methods_.push_back(std::move(method));
      std::ranges::sort(methods_);
      return {};
    } else {
      return std::unexpected{"a method with the signature already exists"};
    }
  });
}

Result<void> Class::DeleteMethod(MethodSignature const& method_signature) {
  return GetMethodFromSignature(method_signature).transform([&](auto m) { methods_.erase(m); });
}

Result<void> Class::RenameMethod(MethodSignature const& method_signature, std::string_view new_name) {
  return GetMethodFromSignature(method_signature).and_then([&](auto m) -> Result<void> {
    if (not GetMethodFromSignature(method_signature.WithName(new_name))) {
      return m->Rename(new_name).transform([&] { std::ranges::sort(methods_); });
    } else {
      return std::unexpected{"a method with the new signature already exists"};
    }
  });
}

Result<void> Class::ChangeParameters(MethodSignature const& method_signature, std::vector<Parameter> parameters) {
  return Check(parameters)
      .and_then([&] { return GetMethodFromSignature(method_signature); })
      .and_then([&](auto m) -> Result<void> {
        if (not GetMethodFromSignature(method_signature.WithParameters(parameters))) {
          return m->ChangeParameters(parameters).transform([&] { std::ranges::sort(methods_); });
        } else {
          return std::unexpected{"a method with the new signature already exists"};
        }
      });
}

Result<void> Class::AddParameter(MethodSignature const& method_signature,
                                 std::string_view parameter_name,
                                 std::string_view parameter_type) {
  return GetMethodFromSignature(method_signature).and_then([&](auto m) -> Result<void> {
    if (not GetMethodFromSignature(method_signature.WithAddedParameter(parameter_type))) {
      return m->AddParameter(parameter_name, parameter_type).transform([&] { std::ranges::sort(methods_); });
    } else {
      return std::unexpected{"a method with the new signature already exists"};
    }
  });
}

Result<void> Class::DeleteParameter(MethodSignature const& method_signature, std::string_view parameter_name) {
  return GetMethodFromSignature(method_signature).and_then([&](auto m) {
    return m->GetParameter(parameter_name).and_then([&](auto p) -> Result<void> {
      if (not GetMethodFromSignature(method_signature.WithoutParameter(m->GetParameterIndex(p)))) {
        return m->RemoveParameter(p).transform([&] { std::ranges::sort(methods_); });
      } else {
        return std::unexpected{"a method with the new signature already exists"};
      }
    });
  });
}

Result<void> Class::DeleteParameters(MethodSignature const& method_signature) {
  return GetMethodFromSignature(method_signature).and_then([&](auto m) -> Result<void> {
    if (not GetMethodFromSignature(method_signature.WithParameters())) {
      return m->ClearParameters().transform([&] { std::ranges::sort(methods_); });
    } else {
      return std::unexpected{"a method with the new signature already exists"};
    }
  });
}

Result<void> Class::ChangeParameterType(MethodSignature const& method_signature,
                                        std::string_view parameter_name,
                                        std::string_view new_type) {
  return GetMethodFromSignature(method_signature).and_then([&](auto m) {
    return m->GetParameter(parameter_name).and_then([&](auto p) -> Result<void> {
      if (not GetMethodFromSignature(method_signature.WithParameterType(m->GetParameterIndex(p), new_type))) {
        return p->ChangeType(new_type).transform([&] { std::ranges::sort(methods_); });
      } else {
        return std::unexpected{"a method with the new signature already exists"};
      }
    });
  });
}

void Class::Move(int new_x, int new_y) {
  position_.x = new_x;
  position_.y = new_y;
}

std::strong_ordering Class::operator<=>(Class const& other) const noexcept {
  return name_ <=> other.name_;
}

bool Class::operator==(Class const& other) const noexcept {
  return name_ == other.name_;
}

} // namespace model

DOCTEST_TEST_SUITE("model::Point") {
  DOCTEST_TEST_CASE("model::Point.Json") {
    [[maybe_unused]] model::Point point{.x = 69, .y = 420};
    nlohmann::json json;
    REQUIRE_NOTHROW(json = point);
    CHECK_EQ(json["x"], point.x);
    CHECK_EQ(json["y"], point.y);
    json["x"] = 42;
    json["y"] = -14;
    REQUIRE_NOTHROW(point = json);
    CHECK_EQ(point.x, 42);
    CHECK_EQ(point.y, -14);
  }
}

DOCTEST_TEST_SUITE("model::Class") {
  DOCTEST_TEST_CASE("model::Class.From") {
    CHECK_FALSE(model::Class::From(""));
    CHECK_FALSE(model::Class::From(" "));
    auto cls = model::Class::From("Class");
    REQUIRE(cls);
    CHECK_EQ(cls->Name(), "Class");
    CHECK(cls->Fields().empty());
    CHECK(cls->Methods().empty());
  }
  DOCTEST_TEST_CASE("model::Class.Rename") {
    model::Class c;
    CHECK_FALSE(c.Rename(" "));
    CHECK(c.Rename("NewName"));
    CHECK_EQ(c.Name(), "NewName");
    CHECK(c.Rename("Name"));
    CHECK_EQ(c.Name(), "Name");
  }
  DOCTEST_TEST_CASE("model::Class.AddField") {
    model::Class c;
    REQUIRE(c.Fields().empty());
    CHECK_FALSE(c.AddField(" ", " "));
    CHECK_FALSE(c.AddField(" ", "int"));
    CHECK_FALSE(c.AddField("name", " "));

    CHECK(c.AddField("name", "type"));
    CHECK_FALSE(c.Fields().empty());
    CHECK_EQ(c.Fields().back().Name(), "name");
    CHECK_EQ(c.Fields().back().Type(), "type");

    CHECK_FALSE(c.AddField("name", "type"));

    CHECK(c.AddField("a", "int"));
    CHECK_EQ(c.Fields().size(), 2);
    CHECK_EQ(c.Fields()[0].Name(), "a");
    CHECK_EQ(c.Fields()[0].Type(), "int");

    CHECK(c.AddField("b", "int"));
    CHECK_EQ(c.Fields().size(), 3);
    CHECK_EQ(c.Fields()[1].Name(), "b");
    CHECK_EQ(c.Fields()[1].Type(), "int");
  }
  DOCTEST_TEST_CASE("model::Class.DeleteField") {
    model::Class c;
    REQUIRE(c.AddField("a", "int"));
    REQUIRE(c.AddField("b", "int"));
    REQUIRE(c.AddField("c", "int"));
    CHECK_FALSE(c.DeleteField(""));
    CHECK_FALSE(c.DeleteField("f"));
    CHECK(c.DeleteField("a"));
    CHECK_EQ(c.Fields().size(), 2);
    CHECK_EQ(c.Fields().front().Name(), "b");
    CHECK_EQ(c.Fields().back().Name(), "c");
    CHECK(c.DeleteField("c"));
    CHECK_EQ(c.Fields().size(), 1);
    CHECK_EQ(c.Fields().front().Name(), "b");
  }
  DOCTEST_TEST_CASE("model::Class.RenameField") {
    model::Class c;
    REQUIRE(c.AddField("a", "int"));
    REQUIRE(c.AddField("b", "str"));
    REQUIRE(c.AddField("c", "any"));
    CHECK_FALSE(c.RenameField("a", "b"));
    CHECK_FALSE(c.RenameField(" ", "d"));
    CHECK_FALSE(c.RenameField("g", "b"));
    CHECK_FALSE(c.RenameField("a", "c"));
    CHECK_FALSE(c.RenameField("a", " "));
    CHECK(c.RenameField("a", "d"));
    CHECK_EQ(c.Fields().front().Name(), "b");
    CHECK_EQ(c.Fields().front().Type(), "str");
    CHECK_EQ(c.Fields().back().Name(), "d");
    CHECK_EQ(c.Fields().back().Type(), "int");
  }
  DOCTEST_TEST_CASE("model::Class.GetField") {
    model::Class c;
    REQUIRE(c.AddField("a", "int"));
    REQUIRE(c.AddField("b", "str"));
    REQUIRE(c.AddField("c", "any"));
    CHECK(c.GetField("a"));
    CHECK(c.GetField("b"));
    CHECK(c.GetField("c"));
    CHECK_FALSE(c.GetField("d"));
    CHECK_FALSE(c.GetField(" "));
    CHECK(c.GetReadOnlyField("a"));
    CHECK(c.GetReadOnlyField("b"));
    CHECK(c.GetReadOnlyField("c"));
    CHECK_FALSE(c.GetReadOnlyField("d"));
    CHECK_FALSE(c.GetReadOnlyField(" "));
  }
  DOCTEST_TEST_CASE("model::Class.AddMethod") {
    model::Class c;
    CHECK_FALSE(c.AddMethod(" ", "void", {}));
    CHECK_FALSE(c.AddMethod("f", " ", {}));
    CHECK_FALSE(c.AddMethod("f", "void", *model::Parameter::MultipleFromString("a:int,a:int")));
    CHECK(c.AddMethod("f", "void", {}));
    REQUIRE_EQ(c.Methods().size(), 1);
    CHECK_EQ(c.Methods().back().Name(), "f");
    CHECK_EQ(c.Methods().back().ReturnType(), "void");
    CHECK(c.Methods().back().Parameters().empty());
    CHECK_FALSE(c.AddMethod("f", "void", {}));
    CHECK(c.AddMethod("f", "void", *model::Parameter::MultipleFromString("a:int,b:int")));
    REQUIRE_EQ(c.Methods().size(), 2);
    CHECK_EQ(c.Methods().back().Name(), "f");
    CHECK_EQ(c.Methods().back().ReturnType(), "void");
    REQUIRE_EQ(c.Methods().back().Parameters().size(), 2);
    CHECK_EQ(c.Methods().back().Parameters()[0].Name(), "a");
    CHECK_EQ(c.Methods().back().Parameters()[0].Type(), "int");
    CHECK_EQ(c.Methods().back().Parameters()[1].Name(), "b");
    CHECK_EQ(c.Methods().back().Parameters()[1].Type(), "int");
    CHECK_FALSE(c.AddMethod("f", "int", *model::Parameter::MultipleFromString("c:int,d:int")));
  }
  DOCTEST_TEST_CASE("model::Class.DeleteMethod") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("g", "int", *model::Parameter::MultipleFromString("d:int")));
    REQUIRE(c.AddMethod("h", "str", *model::Parameter::MultipleFromString("a:int,b:int")));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK_FALSE(c.DeleteMethod(model::MethodSignature{"", {}}));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK_FALSE(c.DeleteMethod(model::MethodSignature{"g", {}}));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK_FALSE(c.DeleteMethod(model::MethodSignature{"h", {}}));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK_FALSE(c.DeleteMethod(model::MethodSignature{"f", {"int"}}));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK_FALSE(c.DeleteMethod(model::MethodSignature{"f", {"int", "int"}}));
    CHECK_EQ(c.Methods().size(), 3);
    CHECK(c.DeleteMethod(model::MethodSignature{"f", {}}));
    CHECK_EQ(c.Methods().size(), 2);
    CHECK_EQ(c.Methods()[0].Name(), "g");
    CHECK_EQ(c.Methods()[1].Name(), "h");
  }
  DOCTEST_TEST_CASE("model::Class.RenameMethod") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("g", "void", *model::Parameter::MultipleFromString("a:int")));
    REQUIRE(c.AddMethod("h", "void", *model::Parameter::MultipleFromString("a:int")));
    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("h(int)")));
    CHECK_FALSE(c.RenameMethod(*m, " "));
    CHECK_FALSE(c.RenameMethod(*m, "g"));
    CHECK(c.RenameMethod(*m, "f"));
    CHECK_EQ(c.Methods()[1].Name(), "f");
    CHECK_EQ(c.Methods()[1].Parameters().size(), 1);
  }
  DOCTEST_TEST_CASE("model::Class.GetMethod") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:int")));
    Result<model::Method> m;
    REQUIRE((m = model::Method::FromString("g()->int")));
    CHECK_FALSE(c.GetReadOnlyMethod(*m));
    CHECK_FALSE(c.GetMethod(*m));
    REQUIRE((m = model::Method::FromString("f(a:int,c:str)->str")));
    CHECK_FALSE(c.GetReadOnlyMethod(*m));
    CHECK_FALSE(c.GetMethod(*m));
    REQUIRE((m = model::Method::FromString("f(a:int)->str")));
    CHECK_FALSE(c.GetReadOnlyMethod(*m));
    CHECK_FALSE(c.GetMethod(*m));

    Result<std::vector<model::Method>::const_iterator> ci;
    Result<std::vector<model::Method>::iterator> i;
    REQUIRE((m = model::Method::FromString("f()->void")));
    REQUIRE((ci = c.GetReadOnlyMethod(*m)));
    REQUIRE((i = c.GetMethod(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "void");
    CHECK((*ci)->Parameters().empty());
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "void");
    CHECK((*i)->Parameters().empty());

    REQUIRE((m = model::Method::FromString("f(a:int,b:int)->str")));
    REQUIRE((ci = c.GetReadOnlyMethod(*m)));
    REQUIRE((i = c.GetMethod(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "str");
    CHECK_EQ((*ci)->Parameters().size(), 2);
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "str");
    CHECK_EQ((*i)->Parameters().size(), 2);

    REQUIRE((m = model::Method::FromString("f(a:int,b:int)->int")));
    REQUIRE((ci = c.GetReadOnlyMethod(*m)));
    REQUIRE((i = c.GetMethod(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "str");
    CHECK_EQ((*ci)->Parameters().size(), 2);
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "str");
    CHECK_EQ((*i)->Parameters().size(), 2);

    REQUIRE((m = model::Method::FromString("f(c:int,d:int)->int")));
    REQUIRE((ci = c.GetReadOnlyMethod(*m)));
    REQUIRE((i = c.GetMethod(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "str");
    CHECK_EQ((*ci)->Parameters().size(), 2);
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "str");
    CHECK_EQ((*i)->Parameters().size(), 2);
  }
  DOCTEST_TEST_CASE("model::Class.GetMethodFromSignature") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:int")));

    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f(str)")));
    CHECK_FALSE(c.GetReadOnlyMethodFromSignature(*m));
    CHECK_FALSE(c.GetMethodFromSignature(*m));
    REQUIRE((m = model::MethodSignature::FromString("f(int,str)")));
    CHECK_FALSE(c.GetReadOnlyMethodFromSignature(*m));
    CHECK_FALSE(c.GetMethodFromSignature(*m));
    REQUIRE((m = model::MethodSignature::FromString("f(int)")));
    CHECK_FALSE(c.GetReadOnlyMethodFromSignature(*m));
    CHECK_FALSE(c.GetMethodFromSignature(*m));

    Result<std::vector<model::Method>::const_iterator> ci;
    Result<std::vector<model::Method>::iterator> i;
    REQUIRE((m = model::MethodSignature::FromString("f()")));
    REQUIRE((ci = c.GetReadOnlyMethodFromSignature(*m)));
    REQUIRE((i = c.GetMethodFromSignature(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "void");
    CHECK((*ci)->Parameters().empty());
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "void");
    CHECK((*i)->Parameters().empty());

    REQUIRE((m = model::MethodSignature::FromString("f(int,int)")));
    REQUIRE((ci = c.GetReadOnlyMethodFromSignature(*m)));
    REQUIRE((i = c.GetMethodFromSignature(*m)));
    CHECK_EQ((*ci)->Name(), "f");
    CHECK_EQ((*ci)->ReturnType(), "str");
    CHECK_EQ((*ci)->Parameters().size(), 2);
    CHECK_EQ((*i)->Name(), "f");
    CHECK_EQ((*i)->ReturnType(), "str");
    CHECK_EQ((*i)->Parameters().size(), 2);
  }
  DOCTEST_TEST_CASE("model::Class.ChangeParameters") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:int")));

    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f()")));
    CHECK_FALSE(c.ChangeParameters(*m, *model::Parameter::MultipleFromString("a:int,b:int")));
    CHECK_FALSE(c.ChangeParameters(*m, *model::Parameter::MultipleFromString("c:int,b:int")));
    CHECK_FALSE(c.ChangeParameters(*m, *model::Parameter::MultipleFromString("c:int,b:int")));
    CHECK(c.ChangeParameters(*m, *model::Parameter::MultipleFromString("d:str")));
    CHECK_EQ(c.Methods().front().Parameters().size(), 1);
    CHECK_EQ(c.Methods().front().Parameters().front().Name(), "d");
    CHECK_EQ(c.Methods().front().Parameters().front().Type(), "str");
  }
  DOCTEST_TEST_CASE("model::Class.AddParameter") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "void", {}));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int")));
    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f()")));
    CHECK_FALSE(c.AddParameter(*m, "b", "int"));
    CHECK_FALSE(c.AddParameter(*m, " ", "int"));
    CHECK_FALSE(c.AddParameter(*m, "b", " "));
    CHECK(c.AddParameter(*m, "a", "str"));
    REQUIRE_FALSE(c.Methods().back().Parameters().empty());
    CHECK_EQ(c.Methods().back().Parameters()[0].Name(), "a");
    CHECK_EQ(c.Methods().back().Parameters()[0].Type(), "str");
  }
  DOCTEST_TEST_CASE("model::Class.DeleteParameter") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int")));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:str")));
    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f(int,str)")));
    CHECK_FALSE(c.DeleteParameter(*m, "b"));
    CHECK_FALSE(c.DeleteParameter(*m, " "));
    CHECK(c.DeleteParameter(*m, "a"));
    CHECK_EQ(c.Methods().back().Parameters().back().Name(), "b");
    CHECK_EQ(c.Methods().back().Parameters().back().Type(), "str");
  }
  DOCTEST_TEST_CASE("model::Class.DeleteParameters") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "str", {}));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:str")));
    REQUIRE(c.AddMethod("g", "str", *model::Parameter::MultipleFromString("a:int,b:str")));
    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f(int)")));
    CHECK_FALSE(c.DeleteParameters(*m));
    REQUIRE((m = model::MethodSignature::FromString("f(int,str)")));
    CHECK_FALSE(c.DeleteParameters(*m));
    REQUIRE((m = model::MethodSignature::FromString("g(int,str)")));
    CHECK(c.DeleteParameters(*m));
    CHECK_EQ(c.Methods().back().Name(), "g");
    CHECK_EQ(c.Methods().back().ReturnType(), "str");
    CHECK(c.Methods().back().Parameters().empty());
  }
  DOCTEST_TEST_CASE("model::Class.ChangeParameterType") {
    model::Class c;
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:int")));
    REQUIRE(c.AddMethod("f", "str", *model::Parameter::MultipleFromString("a:int,b:str")));
    Result<model::MethodSignature> m{std::unexpect, "initial"};
    REQUIRE((m = model::MethodSignature::FromString("f(int,str)")));
    CHECK_FALSE(c.ChangeParameterType(*m, " ", "int"));
    CHECK_FALSE(c.ChangeParameterType(*m, "a", " "));
    CHECK_FALSE(c.ChangeParameterType(*m, "b", "int"));
    CHECK(c.ChangeParameterType(*m, "a", "str"));
    CHECK_EQ(c.Methods().back().Parameters().front().Name(), "a");
    CHECK_EQ(c.Methods().back().Parameters().front().Type(), "str");
  }
  DOCTEST_TEST_CASE("model::Class.Move") {
    model::Class c;
    CHECK_EQ(c.Position().x, 0);
    CHECK_EQ(c.Position().y, 0);
    c.Move(420, 69);
    CHECK_EQ(c.Position().x, 420);
    CHECK_EQ(c.Position().y, 69);
  }
  DOCTEST_TEST_CASE("model::Class.Json") {
    DOCTEST_SUBCASE("Valid") {
      auto json = R"({
        "name": "A",
        "fields": [
          {"name": "x", "type": "int"}
        ],
        "methods": [
          {"name": "f", "return_type": "void", "params": []},
          {"name": "f", "return_type": "void", "params": [{"name": "x", "type": "int"}]}
        ],
        "position": { "x": 37, "y": 73 }
      })"_json;
      model::Class cls;
      REQUIRE_NOTHROW(cls = json);
      CHECK_EQ(cls.Name(), "A");
      REQUIRE_EQ(cls.Fields().size(), 1);
      CHECK_EQ(cls.Fields().front().Name(), "x");
      CHECK_EQ(cls.Fields().front().Type(), "int");
      REQUIRE_EQ(cls.Methods().size(), 2);
      CHECK_EQ(cls.Methods().front().Name(), "f");
      CHECK_EQ(cls.Methods().front().ReturnType(), "void");
      CHECK_EQ(cls.Methods().front().Parameters().size(), 0);
      CHECK_EQ(cls.Methods().back().Name(), "f");
      CHECK_EQ(cls.Methods().back().ReturnType(), "void");
      REQUIRE_EQ(cls.Methods().back().Parameters().size(), 1);
      CHECK_EQ(cls.Methods().back().Parameters().front().Name(), "x");
      CHECK_EQ(cls.Methods().back().Parameters().front().Type(), "int");
      nlohmann::json j;
      REQUIRE_NOTHROW(j = cls);
      CHECK_EQ(j, json);
    }
    DOCTEST_SUBCASE("Invalid") {
      auto json = R"({
        "name": "1",
        "fields": [],
        "methods": [],
        "position": { "x": 37, "y": 73 }
      })"_json;
      model::Class cls;
      REQUIRE_THROWS(cls = json);
    }
  }
  DOCTEST_TEST_CASE("model::Class.Compare") {
    auto a = *model::Class::From("A"), aa = *model::Class::From("A"), b = *model::Class::From("B");
    CHECK_EQ(a, aa);
    CHECK_GE(a, aa);
    CHECK_LE(a, aa);

    CHECK_NE(a, b);
    CHECK_LE(a, b);
    CHECK_LT(a, b);
    CHECK_GE(b, a);
    CHECK_GT(b, a);
  }
  DOCTEST_TEST_CASE("model::Class.Format") {
    auto json = R"({
      "name": "A",
      "fields": [
        {"name": "x", "type": "int"}
      ],
      "methods": [
        {"name": "f", "return_type": "void", "params": []},
        {"name": "f", "return_type": "void", "params": [{"name": "x", "type": "int"}]}
      ],
      "position": { "x": 37, "y": 73 }
    })"_json;
    model::Class cls;
    REQUIRE_NOTHROW(cls = json);
    auto str = std::format("{}", cls);
    auto lines = std::views::split(str, '\n') | std::views::transform([](auto&& s) { return s.size(); }) |
                 std::ranges::to<std::vector>();
    REQUIRE_EQ(lines.size(), cls.Fields().size() + cls.Methods().size() + 1 + 4);
    REQUIRE_GT(lines[0], lines[1]);
    REQUIRE_EQ(lines.front(), lines.back());
    REQUIRE_EQ(lines[0], lines[2]);
    REQUIRE_EQ(lines[2], lines[2 + cls.Fields().size() + 1]);
    auto contents = std::views::filter(lines, [&](std::size_t s) { return s == lines[1]; });
    auto count = std::ranges::distance(contents.begin(), contents.end());
    REQUIRE_EQ(count, 1 + cls.Methods().size() + cls.Fields().size());
  }
}
