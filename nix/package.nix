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

    # Also ship a real .app bundle. omabar needs a CGEventTap for clicks, which
    # macOS gates behind Accessibility. TCC records the *code signing identity*
    # of whatever it grants, and a bare ad-hoc binary's identity is its cdhash --
    # a hash of the exact bytes. Every rebuild produces new bytes, so every
    # rebuild silently invalidated the previous Accessibility grant and the bar
    # went unclickable again. Signing with an explicit designated requirement of
    # `identifier "..."` makes the identity independent of the build, so one
    # grant sticks across rebuilds.
    mkdir -p $out/Applications/Omabar.app/Contents/MacOS
    cp bin/omabar $out/Applications/Omabar.app/Contents/MacOS/Omabar

    cat > $out/Applications/Omabar.app/Contents/Info.plist <<'PLIST'
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
    <dict>
      <key>CFBundleExecutable</key>
      <string>Omabar</string>
      <key>CFBundleIdentifier</key>
      <string>com.nowarelabs.omabar</string>
      <key>CFBundleName</key>
      <string>Omabar</string>
      <key>CFBundleDisplayName</key>
      <string>Omabar</string>
      <key>CFBundlePackageType</key>
      <string>APPL</string>
      <key>CFBundleShortVersionString</key>
      <string>0.1.0</string>
      <key>CFBundleVersion</key>
      <string>0.1.0</string>
      <key>LSMinimumSystemVersion</key>
      <string>13.0</string>
      <!-- Menu-bar style agent: never show a Dock icon or app switcher entry. -->
      <key>LSUIElement</key>
      <true/>
    </dict>
    </plist>
    PLIST

    # NOTE: signing happens in postFixup, not here. stdenv's fixupPhase runs
    # `strip` over $out/bin and $out/Applications *after* installPhase, which
    # rewrites the Mach-O and invalidates any signature made above. The bundle
    # verified fine here but came out of the build with a cdhash identity,
    # which is exactly the failure this bundle exists to prevent.
  '';

  # /usr/bin is not on stdenv's build PATH and codesign ships with macOS rather
  # than as a nixpkgs package, so there is nothing to add to nativeBuildInputs.
  postFixup = ''
    CODESIGN=/usr/bin/codesign
    APP=$out/Applications/Omabar.app

    if [ ! -x "$CODESIGN" ]; then
      echo "WARNING: $CODESIGN not found; Omabar.app is left unsigned." >&2
      echo "WARNING: An unsigned binary has a cdhash identity, so the macOS" >&2
      echo "WARNING: Accessibility permission is revoked on every rebuild" >&2
      echo "WARNING: and the bar stops responding to clicks." >&2
      exit 0
    fi

    # Sign the binary and the bundle around it with the same build-independent
    # requirement, then re-sign so the bundle seals the final binary.
    "$CODESIGN" --force --sign - \
      -r '=designated => identifier "com.nowarelabs.omabar"' \
      "$APP/Contents/MacOS/Omabar"
    "$CODESIGN" --force --sign - \
      -r '=designated => identifier "com.nowarelabs.omabar"' \
      "$APP"
    "$CODESIGN" --verify --deep "$APP"

    # A cdhash identity here means the Accessibility grant will not survive a
    # rebuild, which is the exact bug this bundle exists to fix. Fail the build
    # rather than shipping something that silently loses the grant.
    if ! "$CODESIGN" -d -r- "$APP" 2>&1 \
         | grep -q 'identifier "com.nowarelabs.omabar"'; then
      echo "ERROR: designated requirement is not identifier-based:" >&2
      "$CODESIGN" -d -r- "$APP" >&2
      exit 1
    fi
    echo "Omabar.app signed with a build-independent identity."
  '';

  meta = {
    description = "A macOS status bar configured entirely via Nix";
    homepage = "https://github.com/nowarelabs/omabar";
    license = lib.licenses.mit;
    platforms = lib.platforms.darwin;
    maintainers = [ ];
  };
}
