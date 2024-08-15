{
  description = "unformatter";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }@inputs :
  flake-utils.lib.eachDefaultSystem (system :
    let
      pkgs = nixpkgs.legacyPackages.${system};
      buildPkg = pkgs.callPackage ./nix/build.nix;
      buildPkgCross = tgtPkgs : (tgtPkgs.callPackage ./nix/build.nix {}).overrideAttrs 
        { postInstall = "${tgtPkgs.stdenv.hostPlatform.emulator tgtPkgs.buildPackages} bin/test_unformatter";};
    in {
      packages = {
        default = buildPkg {};
        buildClang = buildPkg {stdenv = pkgs.clangStdenv;};
        buildCrossArm = buildPkgCross pkgs.pkgsCross.aarch64-multiplatform;
        buildCrossRisc = buildPkgCross pkgs.pkgsCross.riscv64;
        buildCrossPpc = buildPkgCross pkgs.pkgsCross.ppc64;
      };
      devShells = {
        default = pkgs.callPackage ./nix/shell.nix { unformatter = self.packages.${system}.default; };
      };
    }
  );
}
