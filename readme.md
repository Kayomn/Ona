# Ona

Game engine.

## Pre-Requisites

In order to build Ona, a few tooling dependencies of the project must first be met.

  * C Compiler
  * D Compiler
  * Dub Package Manager

Additionally, the OpenGL 4 rendering backend Ona depend on third-party libraries.

  * Simple Direct-Media Layer 2 (SDL2)
  * GL Extension Wrangler (GLEW)

Most of these may already be present on the operating systems of most users, as they are common dependencies. Additionally, many of these are shipped as part of most Linux distributions as core utilities.

## Building

Open a terminal and run `dub build` or open the repository root in VS Code and run the default build task. Upon successful compilation, a binary is produced and placed within the `./output` directory.

## Configuration

The engine executable behavior may be configured by creating an `ona.cfg` text configuration file in the working directory.

```ini
[Graphics]
displayTitle = "Ona Demo"
displayWidth = 640
displayHeight = 480
```

A custom configuration file format is used, which supports syntax for data structures not supported in standard INI formats, such as vectors.
