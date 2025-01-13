target_include_directories(uml_editor PRIVATE .)
target_link_libraries(uml_editor
    PRIVATE
    nlohmann_json::nlohmann_json
    doctest::doctest
    PUBLIC
    -lcurses
    -ledit)

target_sources(uml_editor
    PRIVATE
    main.cpp

    cli/readline_controller.cpp
    cli/readline_view.cpp

    commands/base_commands.cpp
    commands/commands.cpp
    commands/completers.cpp
    commands/timeline.cpp

    model/class.cpp
    model/diagram.cpp
    model/field.cpp
    model/method_signature.cpp
    model/method.cpp
    model/parameter.cpp
    model/relationship.cpp
    model/relationship_type.cpp

    utils/io_context.cpp
    utils/utils.cpp)
