# Minimal omabar configuration.
#
# Usage: pass the block under `services.omabar` to the omabar nix-darwin
# module (see README "Quick start"). Turning the module on is enough — the
# default theme in nix/default-theme.nix provides a complete dark bar:
# spaces on the left, front-app in center_left, and clock / volume / battery
# / wifi / media on the right.

{ ... }:
{
  services.omabar.enable = true;
}