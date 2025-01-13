#pragma once

#include "model/diagram.hpp"
#include "utils/utils.hpp"

#include <memory>
#include <span>
#include <string_view>

namespace commands {

///
/// @brief A command class that abstracts the execution and undo of actions
///
///
class Command {
  std::unique_ptr<model::Diagram> prior_state_{nullptr};

public:
  virtual ~Command() noexcept;

  ///
  /// @brief Create a Command from a list of command-line arguments
  ///
  /// @param args
  /// @return error IFF an error occurred during parsing
  ///
  [[nodiscard]] static Result<std::shared_ptr<Command>> From(std::span<std::string_view> args);

  ///
  /// @brief Unapply the command to the diagram
  ///
  /// @param diagram
  /// @return error IFF an error occurred
  ///
  [[nodiscard]] Result<void> Undo(model::Diagram& diagram) const;

  ///
  /// @brief Apply the command to the diagram
  ///
  /// @param diagram
  /// @return error IFF an error occurred during execution
  ///
  [[nodiscard]] virtual Result<void> Execute(model::Diagram& diagram) const = 0;

  ///
  /// @brief Check whether a command should be tracked in history
  ///
  /// @return true if it should be tracked
  /// @return false if it should not be tracked
  ///
  [[nodiscard]] virtual bool Trackable() const noexcept;

  ///
  /// @brief Commit the passed diagram as the held state of the command to be reverted during undo
  ///
  /// @param diagram
  /// @return Result<void>
  ///
  [[nodiscard]] Result<void> Commit(model::Diagram& diagram);
};

///
/// @brief A command which will always have Trackable() return false
///
///
class UntrackableCommand : public Command {
public:
  ~UntrackableCommand() noexcept override;

  [[nodiscard]] bool Trackable() const noexcept override;
};

} // namespace commands
