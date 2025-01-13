#include <memory>
#include <string>
#include <vector>

namespace cli {

template <typename T> struct FreeDeleter {
  static void operator()(T* ptr) noexcept {
    free(ptr);
  }
};

class ReadlineInterface {
  std::string prompt_;
  std::unique_ptr<char, FreeDeleter<char>> buffer_;

public:
  ///
  /// @brief Construct a new Readline Interface
  ///
  /// @param prompt the prompt to display
  ///
  explicit ReadlineInterface(std::string_view prompt);

  ///
  /// @brief Read the next command from the user
  ///
  /// @return true if we read a command
  /// @return false if there are no more commands (EOF)
  ///
  [[nodiscard]] bool ReadCommand();

  ///
  /// @brief Get the current command as a string_view
  ///
  [[nodiscard]] std::string_view GetCommand() const;

  ///
  /// @brief Get the current command in a tokenized format
  ///
  [[nodiscard]] std::vector<std::string_view> GetTokenizedCommand() const;

  ///
  /// @brief Display a message to the user
  ///
  /// @param message what to display
  ///
  void DisplayMessage(std::string_view message) const;

  ///
  /// @brief Add the held command to the history
  ///
  void AddCommandToHistory() const;
};

} // namespace cli
