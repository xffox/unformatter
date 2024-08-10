{ stdenv, cmake, pkg-config, catch2_3, ... }:
stdenv.mkDerivation {
  name = "unformatter";
  src = ./..;
  doCheck = true;
  buildInputs = [
    pkg-config
    catch2_3
  ];
  nativeBuildInputs = [
    cmake
  ];
}
