#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Virtuoso Audio Project Contributors
#
# installer/macos/build-pkg.sh
# Builds and notarizes the Virtuoso Audio macOS installer (.pkg)
#
# Requirements:
#   - Xcode 15+ with Command Line Tools
#   - Apple Developer ID (Installer + Application)
#   - xcrun notarytool (Xcode 14+)
#   - App-specific password in Keychain as item "virtuoso-notarytool-password"
#
# Usage:
#   ./installer/macos/build-pkg.sh [--notarize] [--apple-id you@example.com] [--team-id XXXXXXXXXX]

set -euo pipefail

APPNAME="VirtuosoAudio"
VERSION="1.0.0"
APP_BINARY="build/xcode-universal/Release/$APPNAME.app"
DRIVER_BUNDLE="drivers/macos/VirtuosoAudioPlugin.driver"
PKG_ROOT="/tmp/virtuoso-pkg-root"
PKG_ID="audio.virtuoso.app"
INSTALLER_CERT="Developer ID Installer: Virtuoso Audio Project"
APP_CERT="Developer ID Application: Virtuoso Audio Project"
NOTARIZE=false
APPLE_ID=""
TEAM_ID=""
OUTPUT_PKG="VirtuosoAudio-${VERSION}-macos.pkg"

# Parse args
while [[ $# -gt 0 ]]; do
    case $1 in
        --notarize) NOTARIZE=true ;;
        --apple-id) APPLE_ID="$2"; shift ;;
        --team-id)  TEAM_ID="$2";  shift ;;
    esac
    shift
done

echo "[build-pkg] Building Virtuoso Audio installer v${VERSION}"
echo "[build-pkg] App binary: $APP_BINARY"

# ---------------------------------------------------------------------------
# 1. Build the app and driver if not already built
# ---------------------------------------------------------------------------
if [[ ! -d "$APP_BINARY" ]]; then
    echo "[build-pkg] App not built, running xcodebuild..."
    cmake --preset macos-universal
    xcodebuild -workspace "build/xcode-universal/VirtuosoAudio.xcworkspace" \
               -scheme VirtuosoAudio \
               -configuration Release \
               -destination "generic/platform=macOS" \
               build
fi

# ---------------------------------------------------------------------------
# 2. Codesign the app
# ---------------------------------------------------------------------------
echo "[build-pkg] Signing app..."
codesign --sign "$APP_CERT" \
         --options runtime \
         --entitlements "installer/macos/VirtuosoAudio.entitlements" \
         --timestamp \
         --deep \
         "$APP_BINARY"

# ---------------------------------------------------------------------------
# 3. Codesign the AudioServerPlugIn driver
# ---------------------------------------------------------------------------
echo "[build-pkg] Signing driver..."
codesign --sign "$APP_CERT" \
         --options runtime \
         --timestamp \
         --deep \
         "$DRIVER_BUNDLE"

# ---------------------------------------------------------------------------
# 4. Assemble pkg root
# ---------------------------------------------------------------------------
echo "[build-pkg] Assembling package root..."
rm -rf "$PKG_ROOT"
mkdir -p "$PKG_ROOT/Applications"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/HAL"

cp -R "$APP_BINARY"    "$PKG_ROOT/Applications/"
cp -R "$DRIVER_BUNDLE" "$PKG_ROOT/Library/Audio/Plug-Ins/HAL/"

# ---------------------------------------------------------------------------
# 5. Build component pkg
# ---------------------------------------------------------------------------
COMPONENT_PKG="/tmp/VirtuosoComponent.pkg"
pkgbuild --root   "$PKG_ROOT" \
         --identifier "$PKG_ID" \
         --version   "$VERSION" \
         --install-location "/" \
         "$COMPONENT_PKG"

echo "[build-pkg] Component pkg: $COMPONENT_PKG"

# ---------------------------------------------------------------------------
# 6. Build product archive with productbuild
# ---------------------------------------------------------------------------
productbuild --distribution "installer/macos/distribution.xml" \
             --package-path "/tmp" \
             --sign "$INSTALLER_CERT" \
             "$OUTPUT_PKG"

echo "[build-pkg] Product pkg: $OUTPUT_PKG"

# ---------------------------------------------------------------------------
# 7. Notarize (optional)
# ---------------------------------------------------------------------------
if [[ "$NOTARIZE" == "true" ]]; then
    if [[ -z "$APPLE_ID" || -z "$TEAM_ID" ]]; then
        echo "[build-pkg] ERROR: --apple-id and --team-id required for notarization"
        exit 1
    fi

    echo "[build-pkg] Submitting for notarization..."
    xcrun notarytool submit "$OUTPUT_PKG" \
        --apple-id "$APPLE_ID" \
        --team-id  "$TEAM_ID" \
        --keychain-profile "virtuoso-notarytool-password" \
        --wait

    echo "[build-pkg] Stapling notarization ticket..."
    xcrun stapler staple "$OUTPUT_PKG"
    xcrun stapler validate "$OUTPUT_PKG"
fi

echo "[build-pkg] ✓ Done: $OUTPUT_PKG"
