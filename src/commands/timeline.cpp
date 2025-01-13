#include "timeline.hpp"
#include "commands/base_commands.hpp"
#include "commands/commands.hpp"
#include "model/relationship_type.hpp"
#include "utils/utils.hpp"

#include <doctest/doctest.h>

namespace commands {

[[nodiscard]] Timeline& Timeline::GetInstance() noexcept {
  static Timeline instance;
  return instance;
}

void Timeline::Add(std::shared_ptr<commands::Command>&& cmd) {
  if (cmd->Trackable()) {
    auto iter = timeline_.begin() + static_cast<long>(index_);
    timeline_.erase(iter, timeline_.end());
    timeline_.push_back(std::move(cmd));
    ++index_;
  }
}

[[nodiscard]] Result<std::shared_ptr<commands::Command>> Timeline::Undo() {
  if (index_ == 0) {
    return std::unexpected{"Cannot undo any further"};
  } else {
    return timeline_[--index_];
  }
}

[[nodiscard]] Result<std::shared_ptr<commands::Command>> Timeline::Redo() {
  if (index_ >= timeline_.size()) {
    return std::unexpected{"Cannot redo any further"};
  } else {
    return timeline_[index_++];
  }
}

} // namespace commands

DOCTEST_TEST_SUITE("commands::Timeline") {
  DOCTEST_TEST_CASE("commands::Timeline") {
    commands::Timeline timeline;                                                        //
    auto c0 = std::make_shared<commands::ListAllCommand>(std::tuple<>{});               //
    auto c1 = std::make_shared<commands::AddClassCommand>(std::tuple{"a"});             //
    auto c2 = std::make_shared<commands::AddFieldCommand>(std::tuple{"a", "x", "int"}); //
    auto c3 =
        std::make_shared<commands::AddRelationshipCommand>(std::tuple{"a", "a", model::RelationshipType::Composition});
    CHECK_FALSE(timeline.Undo());
    CHECK_FALSE(timeline.Redo());

    timeline.Add(c0);
    CHECK_FALSE(timeline.Undo());
    CHECK_FALSE(timeline.Redo());

    timeline.Add(c1);
    CHECK_FALSE(timeline.Redo());
    auto res = timeline.Undo();
    CHECK(res);
    CHECK_EQ(res.value(), c1);
    res = timeline.Redo();
    CHECK_EQ(res.value(), c1);

    timeline.Add(c2);
    CHECK_FALSE(timeline.Redo());
    res = timeline.Undo();
    CHECK_EQ(res.value(), c2);
    res = timeline.Undo();
    CHECK_EQ(res.value(), c1);
    CHECK_FALSE(timeline.Undo());
    CHECK(timeline.Redo());

    timeline.Add(c3);
    CHECK_FALSE(timeline.Redo());
    res = timeline.Undo();
    CHECK_EQ(res.value(), c3);
    res = timeline.Undo();
    CHECK_EQ(res.value(), c1);
    CHECK_FALSE(timeline.Undo());
  }
}
