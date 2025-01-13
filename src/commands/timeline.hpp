#pragma once

#include "commands/base_commands.hpp"
#include "utils/utils.hpp"

#include <vector>

namespace commands {

class Timeline {
  std::vector<std::shared_ptr<commands::Command>> timeline_;
  std::size_t index_{0};

public:
  ///
  /// @brief Singleton for Timeline
  ///
  [[nodiscard]] static Timeline& GetInstance() noexcept;

  ///
  /// @brief Add a new command to the timeline
  ///
  /// Any commands to the "right" in the timeline are removed and the new command is added to the end
  ///
  /// @param cmd the command to add
  ///
  void Add(std::shared_ptr<commands::Command>&& cmd);

  ///
  /// @brief Move left in the timeline, returning the command to be undone
  ///
  /// @return Error if undo cannot be performed, else the command to then invoke ->Undo() on
  ///
  [[nodiscard]] Result<std::shared_ptr<commands::Command>> Undo();

  ///
  /// @brief Move right in the timeline, returning the command to be re-executed
  ///
  /// @return Error if redo cannot be performed, else the command to then invoke ->Execute() on
  ///
  [[nodiscard]] Result<std::shared_ptr<commands::Command>> Redo();
};

} // namespace commands
