{ pkgs, stdenv, callPackage, clang-tools, unformatter, python3Packages, ... }@inp :
pkgs.mkShell.override { inherit stdenv; } {
  inputsFrom = [ unformatter ];
  packages = [
    clang-tools
  ];
}
