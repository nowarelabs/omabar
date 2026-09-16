{ lib, stdenv, clang, apple-sdk_15, src }:

stdenv.mkDerivation {
  pname = "omabar";
  version = "0.1.0";

  inherit src;

  buildInputs = [
    clang
    apple-sdk_15
  ];

  buildPhase = ''
    cd src && make -j4
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp bin/omabar $out/bin/
  '';

  meta = {
    description = "A macOS status bar configured entirely via Nix";
    homepage = "https://github.com/anomalyco/omabar";
    license = lib.licenses.mit;
    platforms = lib.platforms.darwin;
    maintainers = [ ];
  };
}