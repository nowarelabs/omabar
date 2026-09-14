{
  description = "A macOS status bar configured entirely via Nix";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachSystem [ "aarch64-darwin" "x86_64-darwin" ] (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        packages.omabar = pkgs.stdenv.mkDerivation {
          pname = "omabar";
          version = "0.1.0";
          src = ./.;
          buildInputs = with pkgs.darwin.apple_sdk.frameworks; [
            Carbon
            AppKit
            QuartzCore
            CoreAudio
            CoreWLAN
            CoreVideo
            IOKit
            CoreText
            ImageIO
            CoreServices
          ] ++ (with pkgs; [
            darwin.IOKit
          ]);
          buildPhase = ''
            cd src && make -j4
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp bin/omabar $out/bin/
          '';
        };

        defaultPackage = self.packages.${system}.omabar;

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            clang
          ] ++ (with pkgs.darwin.apple_sdk.frameworks; [
            Carbon
            AppKit
            QuartzCore
            CoreAudio
            CoreWLAN
            CoreVideo
            IOKit
            CoreText
            ImageIO
            CoreServices
          ]);
        };
      }
    );

  nixosModules.omabar = import ./nix/module.nix;
}