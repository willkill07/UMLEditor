#include "model/diagram.hpp"

#include <variant>
#include <vector>

template <typename T> using Iter = std::vector<T>::const_iterator;

namespace commands {

///
/// @brief A completer for classes
///
struct ClassCompleter {
  /// const-reference to a diagram
  std::reference_wrapper<model::Diagram const> diagram;
  /// held name for match
  std::string_view name;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
  /// get the class' iterator
  [[nodiscard]] Result<Iter<model::Class>> Get() const;
};

///
/// @brief A completer for fields
///
struct FieldCompleter {
  /// held iterator to class to match fields
  Result<Iter<model::Class>> iter;
  /// held name for match
  std::string_view name;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
  /// get the field's iterator
  [[nodiscard]] Result<Iter<model::Field>> Get() const;
};

///
/// @brief A completer for methods
///
struct MethodCompleter {
  /// held iterator to class to match methods
  Result<Iter<model::Class>> iter;
  /// held name for match
  std::string_view signature;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
  /// get the method's iterator
  [[nodiscard]] Result<Iter<model::Method>> Get() const;
};

///
/// @brief A completer for parameters
///
struct ParameterCompleter {
  /// held iterator to method to match parameters
  Result<Iter<model::Method>> iter;
  /// held name for match
  std::string_view name;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
  /// get the parameter's iterator
  [[nodiscard]] Result<Iter<model::Parameter>> Get() const;
};

///
/// @brief A completer for relationship sources
///
struct RelationshipSourceCompleter {
  /// const-reference to a diagram
  std::reference_wrapper<model::Diagram const> diagram;
  /// held name for match
  std::string_view source;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
};

///
/// @brief A completer for relationship destinations
///
struct RelationshipDestinationCompleter {
  /// const-reference to a diagram
  std::reference_wrapper<model::Diagram const> diagram;
  /// held name for match
  std::string_view source;
  /// held name for match
  std::string_view dest;
  /// returns a list of candidates
  [[nodiscard]] std::vector<std::string> Candidates() const;
  /// get the relationship's iterator
  [[nodiscard]] Result<Iter<model::Relationship>> Get() const;
};

///
/// @brief A completer for relationship types
///
struct RelationshipTypeCompleter {
  [[nodiscard]] std::vector<std::string> Candidates() const;
};

///
/// @brief The completer is a variant of all possible completers (or none)
///
using Completer = std::variant<std::monostate,
                               ClassCompleter,
                               FieldCompleter,
                               MethodCompleter,
                               ParameterCompleter,
                               RelationshipSourceCompleter,
                               RelationshipDestinationCompleter,
                               RelationshipTypeCompleter>;

} // namespace commands
