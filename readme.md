# About

Utilities to work with structured data in a type safe manner. As much as possible statically known structure should be checked at compile time. Runtime known structure uses optional-like types.

The utilities work with arrays of data. They allow to select subranges, read and write to the subranges, while keeping these operations checked at compile time or runtime.

## Example

See [sample](sample/main.cpp) for an IPv6 packet example.

# Build

CMake is used for builds.

    mkdir _build
    cmake -B _build
    cmake --build _build

## Nix

The project can also be used as a Nix flake.
