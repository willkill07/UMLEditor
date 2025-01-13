#pragma once

#include "commands/base_commands.hpp"
#include "commands/metaprogramming.hpp"

namespace commands {

#define DefineCommand(Name, Text)                                               \
  struct Name : public Command {                                                \
    static constexpr std::string_view CommandName = Text;                       \
    ~Name() override = default;                                                 \
    inline explicit Name(TupleFor<Name> params) : args{std::move(params)} {     \
    }                                                                           \
    [[nodiscard]] Result<void> Execute(model::Diagram& diagram) const override; \
    TupleFor<Name> args;                                                        \
  }

#define DefineUntrackableCommand(Name, Text)                                    \
  struct Name : public UntrackableCommand {                                     \
    static constexpr std::string_view CommandName = Text;                       \
    ~Name() override = default;                                                 \
    inline explicit Name(TupleFor<Name> params) : args{std::move(params)} {     \
    }                                                                           \
    [[nodiscard]] Result<void> Execute(model::Diagram& diagram) const override; \
    TupleFor<Name> args;                                                        \
  }

DefineCommand(LoadCommand, "load [filename]");
DefineUntrackableCommand(SaveCommand, "save [filename]");
DefineUntrackableCommand(ListAllCommand, "list all");
DefineUntrackableCommand(ListClassesCommand, "list classes");
DefineUntrackableCommand(ListRelationshipsCommand, "list relationships");
DefineUntrackableCommand(ListClassCommand, "list class [class_name]");
DefineUntrackableCommand(HelpCommand, "help");
DefineUntrackableCommand(ExitCommand, "exit");
DefineUntrackableCommand(UndoCommand, "undo");
DefineUntrackableCommand(RedoCommand, "redo");
DefineCommand(AddClassCommand, "class add [name]");
DefineCommand(RemoveClassCommand, "class remove [class_name]");
DefineCommand(RenameClassCommand, "class rename [class_name] [name]");
DefineCommand(MoveClassCommand, "class move [class_name] [int] [int]");
DefineCommand(AddFieldCommand, "field add [class_name] [name] [type]");
DefineCommand(RemoveFieldCommand, "field remove [class_name] [field_name]");
DefineCommand(RenameFieldCommand, "field rename [class_name] [field_name] [name]");
DefineCommand(RetypeFieldCommand, "field retype [class_name] [field_name] [type]");
DefineCommand(AddMethodCommand, "method add [class_name] [method_definition]");
DefineCommand(RemoveMethodCommand, "method remove [class_name] [method_signature]");
DefineCommand(RenameMethodCommand, "method rename [class_name] [method_signature] [name]");
DefineCommand(ChangeReturnTypeCommand, "method change-return-type [class_name] [method_signature] [type]");
DefineCommand(AddParameterCommand, "parameter add [class_name] [method_signature] [name] [type]");
DefineCommand(RemoveParameterCommand, "parameter remove [class_name] [method_signature] [param_name]");
DefineCommand(RenameParameterCommand, "parameter rename [class_name] [method_signature] [param_name] [name]");
DefineCommand(RetypeParameterCommand, "parameter retype [class_name] [method_signature] [param_name] [type]");
DefineCommand(ClearParametersCommand, "parameters clear [class_name] [method_signature]");
DefineCommand(SetParametersCommand, "parameters set [class_name] [method_signature] [param_list]");
DefineCommand(AddRelationshipCommand, "relationship add [class_name] [class_name] [relationship_type]");
DefineCommand(RemoveRelationshipCommand, "relationship remove [class_source] [class_destination]");
DefineCommand(ChangeSourceCommand, "relationship change source [class_source] [class_destination] [class_name]");
DefineCommand(ChangeDestinationCommand,
              "relationship change destination [class_source] [class_destination] [class_name]");
DefineCommand(ChangeTypeCommand, "relationship change type [class_source] [class_destination] [relationship_type]");

#undef DefineCommand
#undef DefineUntrackableCommand

using AllCommands = std::tuple<
    // Class Commands
    AddClassCommand,
    RemoveClassCommand,
    RenameClassCommand,
    // Fields Commands
    AddFieldCommand,
    RemoveFieldCommand,
    RenameFieldCommand,
    RetypeFieldCommand,
    // Method Commands
    AddMethodCommand,
    RemoveMethodCommand,
    RenameMethodCommand,
    ChangeReturnTypeCommand,
    // Parameter Commands
    AddParameterCommand,
    RemoveParameterCommand,
    RenameParameterCommand,
    RetypeParameterCommand,
    ClearParametersCommand,
    SetParametersCommand,
    // Relationship Commands
    AddRelationshipCommand,
    RemoveRelationshipCommand,
    ChangeSourceCommand,
    ChangeDestinationCommand,
    ChangeTypeCommand,
    // List Commands
    ListAllCommand,
    ListClassesCommand,
    ListRelationshipsCommand,
    ListClassCommand,
    // File Commands
    LoadCommand,
    SaveCommand,
    HelpCommand,
    ExitCommand,
    RedoCommand,
    UndoCommand>;

static constexpr auto CommandStrings{[]<std::size_t... Is>(std::index_sequence<Is...>) {
  return std::array{std::tuple_element_t<Is, AllCommands>::CommandName...};
}(std::make_index_sequence<std::tuple_size_v<AllCommands>>{})};

} // namespace commands
