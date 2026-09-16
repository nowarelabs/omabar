/* nix/plugin-sdk.nix — exposes the Swift plugin SDK as a Swift Package.

   The output is a SwiftPM package ("Omabar") containing
   OmabarPlugin.swift + OmabarClient.swift.  Builds the module once so
   the `.swiftmodule` is available for direct import, and also ships
   the raw sources under `$out/share/omabar-sdk` so plugin authors can
   add the output path to their own Package.swift dependencies.

   The built SDK module lives at:
     $out/lib/swift/Omabar.swiftmodule
     $out/lib/libOmabar.a
*/

{
  lib,
  stdenv,
  swift,
  swiftpm,
  src,
}:

stdenv.mkDerivation {
  pname = "omabar-plugin-sdk";
  version = "0.1.0";

  inherit src;
  sourceRoot = "source/plugins/sdk";

  buildInputs = [
    swift
    swiftpm
  ];

  configurePhase = ''
    runHook preConfigure
    mkdir -p "$NIX_BUILD_TOP/.build"
    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild
    swift build -c release --scratch-path "$NIX_BUILD_TOP/.build"
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p "$out/lib/swift" "$out/share/omabar-sdk"
    cp -r "$NIX_BUILD_TOP/.build/release/Omabar.swiftmodule" "$out/lib/swift/"
    cp -r "$NIX_BUILD_TOP/.build/release/Omabar.build" "$out/lib/swift/" 2>/dev/null || true
    cp "$NIX_BUILD_TOP/.build/release/Omabar.swiftdoc" "$out/lib/swift/" 2>/dev/null || true
    cp "$NIX_BUILD_TOP/.build/release/ModuleInfo.json" "$out/lib/" 2>/dev/null || true
    cp -r . "$out/share/omabar-sdk/"
    runHook postInstall
  '';

  meta = {
    description = "Swift plugin SDK (protocol + IPC client) for omabar";
    homepage = "https://github.com/anomalyco/omabar";
    license = lib.licenses.mit;
    platforms = lib.platforms.darwin;
    maintainers = [ ];
  };
}