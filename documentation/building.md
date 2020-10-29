# Build System Documentation

## Build System Architecture

Ona uses a non-conventional build system written in stock Python 3. The build system does not depend on any third-party libraries or tooling that is distinct from a default Python 3 installation. Due to this, there is a massive dependency hell avoided, as most everyone likely already has Python 3 installed on their own machines.

The codebase of Ona is based inside of `ona/` and is segmented into distinct modules of code. These modules are **not** permitted to have any kind of cylical dependencies on eachother, but may share dependencies between one another. For example, `ona/collections` and `ona/engine` both have a dependency on `ona/core`, however this means that `ona/core` therefore cannot depend on either of them itself.

## Dependencies

Building a module is simple, however it is expected that you have some familiarity with the Linux command line and have all the required dependencies to build Ona. Currently, the dependencies are:

  * Python3
  * Clang / Clang++
  * OpenGL
  * SDL2
  * GLEW
  * A unix-compatible operating system.

The latter dependency means that Ona cannot currently be built on Windows. This will be fixed in the near future, one a suitable implementation of the `ona/core/os.cpp` implementation file is created that supports Windows.

## Building

To build Ona, open a terminal session located in the root of the project folder and type `./build.py engine`. This will begin the process of building the `ona/engine` module and all of its dependencies, producing an executable program once complete.

Likewise, should any other module wish to be built instead of `ona/engine`, you only need replace the command line argument. Examples include `./build.py core` - which will build the core utils that Ona depends upon.
