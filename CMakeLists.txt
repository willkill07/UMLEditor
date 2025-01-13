cmake_minimum_required(VERSION 3.25.0)

project(UMLEditor VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS On)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)

set(ENABLE_COVERAGE Off CACHE BOOL "")
set(ENABLE_SANITIZERS Off CACHE BOOL "")

if(ENABLE_COVERAGE)
  message(STATUS "Adding support for coverage")
  add_compile_options(
    # universal options
    --coverage
    -O0
    # debugging symbols
    $<$<CXX_COMPILER_ID:AppleClang>:-g>
    $<$<CXX_COMPILER_ID:Clang>:-g>
    $<$<CXX_COMPILER_ID:GNU>:-g3>)
  add_link_options(--coverage)
endif()
if(ENABLE_SANITIZERS)
message(STATUS "Adding support for sanitizers")
  add_compile_options(
    # universal options
    -O0
    -fsanitize=address,undefined
    # debugging symbols
    $<$<CXX_COMPILER_ID:Clang>:-g>
    $<$<CXX_COMPILER_ID:AppleClang>:-g>
    $<$<CXX_COMPILER_ID:GNU>:-g3>
    # compiler-specific flags
    $<$<CXX_COMPILER_ID:Clang>:-fsanitize=integer,nullability,implicit-conversion,nullability>
    $<$<CXX_COMPILER_ID:AppleClang>:-fsanitize=integer,nullability,implicit-conversion,nullability>
    $<$<CXX_COMPILER_ID:GNU>:-fsanitize=shift,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,bounds-strict,alignment,object-size,nonnull-attribute,returns-nonnull-attribute,bool,enum,vptr,pointer-overflow,builtin>)
  add_link_options(-fsanitize=address,undefined)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(
    $<$<CXX_COMPILER_ID:Clang>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>
    $<$<CXX_COMPILER_ID:AppleClang>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>
    $<$<CXX_COMPILER_ID:GNU>:_GLIBCXX_DEBUG>
    $<$<CXX_COMPILER_ID:GNU>:_GLIBCXX_DEBUG_PEDANTIC>
    $<$<CXX_COMPILER_ID:GNU>:_GLIBCXX_ASSERTIONS>
    $<$<CXX_COMPILER_ID:GNU>:_GLIBCXX_SANITIZE_VECTOR>)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  add_compile_definitions(DOCTEST_CONFIG_DISABLE)
else()
  enable_testing()
  add_test(NAME tests COMMAND uml_editor --tests --reporters=junit --out=${CMAKE_SOURCE_DIR}/junit.xml)
endif()

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  GIT_TAG 2d42229f4d68c6f86f37468b84ac65e86b815bbb
  SYSTEM
  EXCLUDE_FROM_ALL
)

CPMAddPackage(
  NAME doctest
  GITHUB_REPOSITORY doctest/doctest
  GIT_TAG v2.4.11
  SYSTEM
  EXCLUDE_FROM_ALL
  OPTIONS
  "DOCTEST_WITH_MAIN_IN_STATIC_LIB Off"
  "DOCTEST_NO_INSTALL On"
)

add_executable(uml_editor)
add_subdirectory(src)

install(TARGETS uml_editor RUNTIME)
include(CPack)

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
