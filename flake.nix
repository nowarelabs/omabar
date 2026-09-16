{
  description = "A macOS status bar configured entirely via Nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

    nix-darwin = {
      url = "github:LnL7/nix-darwin";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      nix-darwin,
      ...
    }:
    let
      forSystems =
        systems: f:
        nixpkgs.lib.genAttrs systems (
          system:
          let
            pkgs = nixpkgs.legacyPackages.${system};
          in
          f { inherit pkgs system; }
        );
      supportedSystems = [
        "aarch64-darwin"
        "x86_64-darwin"
      ];
    in
    {
      packages = forSystems supportedSystems (
        { pkgs, system }:
        {
          omabar = pkgs.callPackage ./nix/package.nix { src = self; };
          omabar-plugin-sdk = pkgs.callPackage ./nix/plugin-sdk.nix { src = self; };
          default = pkgs.callPackage ./nix/package.nix { src = self; };
        }
      );

      devShells = forSystems supportedSystems (
        { pkgs, ... }:
        {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              clang
              apple-sdk_15
            ];
          };
        }
      );

      # nix-darwin module — exposes the omabar package via overlay so
      # consumers only need services.omabar.enable = true; and the
      # bar config declaratively.
      nixosModules.omabar =
        { config, lib, pkgs, ... }:
        let
          cfg = config.services.omabar;
        in
        {
          imports = [ ./nix/module.nix ];

          # Expose pkgs.omabar so module's launchd agent / plugins work
          # without consumers manually applying the overlay.
          nixpkgs.overlays = [ self.overlays.default ];
        };

      # alias used by some nix-darwin wiring
      darwinModules.omabar = self.nixosModules.omabar;

      overlays.default = final: prev: {
        omabar = final.callPackage ./nix/package.nix { src = self; };
        omabar-plugin-sdk = final.callPackage ./nix/plugin-sdk.nix { src = self; };
      };
      overlays.omabar = self.overlays.default;
    };
}