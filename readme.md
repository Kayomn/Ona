# Ona

Game engine.

## Pre-Requisites

In order to build Ona, a few tooling dependencies of the project must first be met.

  * Clang / Clang++
  * Python3
  * `ar` Unix Archiving Tool

Further to this, Ona depends on some third-party libraries.

  * Unix Dynamic Linking Library (dl)
  * Simple Direct-Media Layer 2 (SDL2)
  * OpenGL (GL)
  * GL Extension Wrangler (GLEW)
  * Lua 5.3 (lua5.3)
  * Posix Threading Library (pthread)

Most of these may already be present on the operating systems of most users, as they are common dependencies. Additionally, many of these are shipped as part of most Linux distributions as core utilities.

## Building

Open a terminal and run `./build-engine.py`. If the project has already been previously compiled, it is recommended to run `./clean.py` first to purge the previous build binaries entirely. For general development, cleaning is not necessary as the build script does incremental builds.
