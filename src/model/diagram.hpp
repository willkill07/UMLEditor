#pragma once

#include "model/class.hpp"
#include "model/relationship.hpp"
#include "model/relationship_type.hpp"

#include "utils/utils.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>

namespace model {

class Diagram {
  std::vector<Class> classes_;
  std::vector<Relationship> relationships_;

  //NOLINTBEGIN(readability-identifier-naming)
  friend void to_json(nlohmann::json&, Diagram const&);
  friend void from_json(nlohmann::json const&, Diagram&);
  //NOLINTEND(readability-identifier-naming)
public:
  ///
  /// @brief Singleton for Diagram
  ///
  [[nodiscard]] static Diagram& GetInstance() noexcept;

  ///
  /// @brief Get an iterator to a corresponding class
  ///
  /// @param name
  /// @return Error if the name is invalid or the class doesn't exist
  ///
  [[nodiscard]] Result<std::vector<Class>::iterator> GetClass(std::string_view name);

  ///
  /// @brief Get an iterator to a corresponding class
  ///
  /// @param name
  /// @return Error if the name is invalid or the class doesn't exist
  ///
  [[nodiscard]] Result<std::vector<Class>::const_iterator> GetClass(std::string_view name) const;

  ///
  /// @brief Get an iterator to a corresponding relationship
  ///
  /// @param src
  /// @param dst
  /// @return Error if either name is invalid or the relationship doesn't exist
  ///
  [[nodiscard]] Result<std::vector<Relationship>::iterator> GetRelationship(std::string_view src, std::string_view dst);

  ///
  /// @brief Get an iterator to a corresponding relationship
  ///
  /// @param src
  /// @param dst
  /// @return Error if either name is invalid or the relationship doesn't exist
  ///
  [[nodiscard]] Result<std::vector<Relationship>::const_iterator> GetReadOnlyRelationship(std::string_view src,
                                                                                          std::string_view dst) const;

  ///
  /// @brief Add a new class to the diagram
  ///
  /// @param name
  /// @return Error IFF adding failed
  ///
  [[nodiscard]] Result<void> AddClass(std::string_view name);

  ///
  /// @brief Delete a class from the diagram
  ///
  /// @param name
  /// @return Error IFF deleting failed
  ///
  [[nodiscard]] Result<void> DeleteClass(std::string_view name);

  ///
  /// @brief Rename a class in the diagram
  ///
  /// @param old_name
  /// @param new_name
  /// @return Error IFF renaming failed
  ///
  [[nodiscard]] Result<void> RenameClass(std::string_view old_name, std::string_view new_name);

  ///
  /// @brief Add a relationship to the diagram
  ///
  /// @param source
  /// @param destination
  /// @param type
  /// @return Error IFF adding failed
  ///
  [[nodiscard]] Result<void>
  AddRelationship(std::string_view source, std::string_view destination, RelationshipType type);

  ///
  /// @brief Delete a relationship from the diagram
  ///
  /// @param source
  /// @param destination
  /// @return Error IFF deleting failed
  ///
  [[nodiscard]] Result<void> DeleteRelationship(std::string_view source, std::string_view destination);

  ///
  /// @brief Change the source of a relationship within the diagram
  ///
  /// @param source
  /// @param destination
  /// @param new_source
  /// @return Error IFF changing the source failed
  ///
  [[nodiscard]] Result<void>
  ChangeRelationshipSource(std::string_view source, std::string_view destination, std::string_view new_source);

  ///
  /// @brief Change the destination of a relationship within the diagram
  ///
  /// @param source
  /// @param destination
  /// @param new_destination
  /// @return Error IFF changing the destination failed
  ///
  [[nodiscard]] Result<void> ChangeRelationshipDestination(std::string_view source,
                                                           std::string_view destination,
                                                           std::string_view new_destination);

  ///
  /// @brief Load a file and replace this Diagram's contents
  ///
  /// @param file_name
  /// @return Error IFF loading the file failed (including validation)
  ///
  [[nodiscard]] Result<void> Load(std::string_view file_name);

  ///
  /// @brief Save this diagram's representation to a file
  ///
  /// @param file_name
  /// @return Error IFF saving the file failed
  ///
  [[nodiscard]] Result<void> Save(std::string_view file_name);

  ///
  /// @brief Get the names of the classes of the diagram
  ///
  /// @return std::vector<std::string>
  ///
  [[nodiscard]] std::vector<std::string> GetClassNames() const;

  ///
  /// @brief Get the classes of the diagram
  ///
  /// @return std::vector<Class> const&
  ///
  [[nodiscard]] std::vector<Class> const& GetClasses() const noexcept;

  ///
  /// @brief Get the relationships of the diagram
  ///
  /// @return std::vector<Relationship> const&
  ///
  [[nodiscard]] std::vector<Relationship> const& GetRelationships() const noexcept;
};

} // namespace model

template <> struct std::formatter<model::Diagram> {
  std::string_view format_text;
  template <typename FormatParseContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  constexpr inline auto parse(FormatParseContext& ctx) {
    auto start = ctx.begin();
    auto end = start;
    while (*end != '}') {
      ++end;
    }
    format_text = std::string_view{start, end};
    return end;
  }
  template <typename FormatContext>
  //NOLINTNEXTLINE(readability-identifier-naming)
  auto format(model::Diagram const& obj, FormatContext& ctx) const {
    for (char letter : format_text) {
      if (letter == 'c') {
        for (model::Class const& c : obj.GetClasses()) {
          ctx.advance_to(std::format_to(ctx.out(), "{}\n", c));
        }
      } else if (letter == 'r') {
        for (model::Relationship const& r : obj.GetRelationships()) {
          ctx.advance_to(std::format_to(ctx.out(), "{}\n", r));
        }
      }
    }
    return ctx.out();
  }
};
