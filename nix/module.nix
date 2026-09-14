{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.omabar;
  omabar = pkgs.omabar;
in {
  options.services.omabar = {
    enable = mkEnableOption "omabar status bar";
  };

  config = mkIf cfg.enable {
    launchd.agents.omabar = {
      enable = true;
      config.ProgramArguments = [ "${omabar}/bin/omabar" ];
      config.RunAtLoad = true;
    };
  };
}