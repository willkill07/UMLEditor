# UML Editor

[![CI](https://github.com/willkill07/UMLEditor/actions/workflows/ci.yml/badge.svg)](https://github.com/willkill07/UMLEditor/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/willkill07/UMLEditor/graph/badge.svg?token=W821QNJ9TV)](https://codecov.io/gh/willkill07/UMLEditor) 

## Cloning

* `git clone https://www.github.com:willkill07/UMLEditor.git`

## Building

There are several presets defined for building:
- Debug
- RelWithDebInfo
- Release
- MinSizeRel (has tests disabled)

### Linux

1. Install Dependencies (system package manager)
   * `libedit` (may be `libedit` or `libedit-dev`)
   * `cmake` >= 3.25
   * `ninja`
   * `gcc` >= 14

2. Build
   * `CMAKE_BUILD_TYPE=Release cmake --workflow --preset Default`

### macOS

1. Ensure AppleClang >= 16.0.0 is installed

2. Install Dependencies (using `brew`)
   * `brew install cmake ninja libedit`

3. Build
   * `CMAKE_BUILD_TYPE=Release cmake --workflow --preset Default`

## Running

`./build/uml_editor` can be invoked from the command-line.

`--cli` must be passed as the sole argument if you wish to drop into the command-line interface.

## Testing

Tests are automatically run for all workflow presets so long as they exist.

You can explicitly invoke tests in one of two ways:

* `ctest --test-dir build`
* `./build/uml_editor --tests`
