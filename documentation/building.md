# Build System Documentation

## Build System Architecture

Ona uses a non-conventional build system written in stock Python 3. As a result, only a Python 3.x and C++20 conformant Clang installation are necessary tools to build the project.

The engine codebase of Ona is placed inside of `ona/`  and is segmented into distinct modules of code. These modules are **not** permitted to have any kind of cylical dependencies on eachother, but may share dependencies between one another. For example, `ona/collections` and `ona/engine` both have a dependency on `ona/core`, however this means that `ona/core` therefore cannot depend on either of them itself.

## Modules

A module is comprised of a module folder (inside `ona/`), a module header (`.hpp` file inside the module folder), and a module configuration file (`.json` file placed in `ona/` with the same name as the module folder). Modules may only be statically linked together with one another, either into an executable or library.

```json
{
  "isExecutable": true,
  "libraries": [
    "dl",
    "SDL2",
    "GL",
    "GLEW",
    "lua5.3"
  ],
  "dependencies": [
    "core",
    "collections"
  ]
}
```

Above is an example of a module configuration file. The file itself dictates the target type, as well as any internal dependencies (under `dependencies`) on top of any external ones (under `libraries`).

## Extensions

Alongside modules, Ona also supports runtime-loaded extensions in the form of shared libraries (otherwise known as dynamic-link libraries on Windows). Currently, extensions are only permitted to be single-file C++ programs that are placed within `ext/`. During the build process, the script will enter the extensions directory and compile and link each extension source file into its own shared object / DLL.

## Dependencies

Building the project is simple, however, it is also expected that you have some familiarity with the Linux command line and have all the required external dependencies. Currently, the dependencies are:

  * Python3
  * Clang / Clang++
  * OpenGL (Windows / Linux)
  * SDL2 (Windows / Linux)
  * GLEW (Windows / Linux)

## Building

To build Ona, open a terminal session located in the root of the project folder and type `./build.py`.
