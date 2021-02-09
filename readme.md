# Ona

Game engine.

## Pre-Requisites

In order to build Ona, a few tooling dependencies of the project must first be met.

  * Clang / Clang++ with C++17
  * Python3
  * `ar` Unix Archiving Tool

Further to this, Ona depends on some third-party libraries.

  * Unix Dynamic Linking Library (dl)
  * Simple Direct-Media Layer 2 (SDL2)
  * OpenGL (GL)
  * GL Extension Wrangler (GLEW)
  * Posix Threading Library (pthread)

Most of these may already be present on the operating systems of most users, as they are common dependencies. Additionally, many of these are shipped as part of most Linux distributions as core utilities.

## Building

Open a terminal and run `./build.py`. If the project has already been previously compiled, it is recommended to run `./clean.py` first to purge the previous build binaries entirely. For general development, cleaning is not necessary as the build script does incremental builds.

## Modules

Ona supports the loading of external code modules at runtime in order to introduce additional functionality to the engine. Modules may implement the following:

  * Additional graphics servers.
  * Additional resource file format support.
  * Scene graphs and entity component systems.
  * User-interfaces.
  * Scripting languages.
  * An entire game.

During engine initialization, Ona will look for a directory named `modules` in the current working directory. Assuming it can be found, Ona will proceed to try and load each file in the directory as a module.
