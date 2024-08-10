{ pkgs, callPackage, clang-tools, unformatter, ... }@inp :
pkgs.mkShell.override {} {
  inputsFrom = [ unformatter ];
  packages = [
    clang-tools
  ];
}
