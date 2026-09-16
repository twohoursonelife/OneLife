#!/bin/sh

#
# Modification History
#
# 2026-September-16    hobby-dev
# Created.  Builds and packages a macOS client release: configure, make,
# then build/makeReleaseFolder.  Checks for build dependencies first and
# tells you what's missing and how to install it -- it never installs
# anything itself.
#
# Usage:
#     build/macOSX/buildRelease.sh [release_name]
#     MACOSX_ARCHS="arm64 x86_64" build/macOSX/buildRelease.sh v20327
#
# release_name defaults to v<../OneLifeData7/dataVersionNumber.txt>.
#

set -e

cd "$(dirname "$0")/../.."   # OneLife repo root

fail() {
    echo >&2
    echo "$0: $1" >&2
    shift
    for line in "$@" ; do echo "$line" >&2 ; done
    exit 1
}


##### Sibling repos: OneLife, minorGems and OneLifeData7 must sit side by side.

[ -d ../minorGems ] || fail "../minorGems not found." \
    "Clone it next to this repo with:" \
    "    git clone https://github.com/twohoursonelife/minorGems ../minorGems"

[ -d ../OneLifeData7 ] || fail "../OneLifeData7 not found." \
    "Clone it next to this repo with:" \
    "    git clone https://github.com/twohoursonelife/OneLifeData7 ../OneLifeData7"


##### Xcode Command Line Tools

if ! /usr/bin/xcrun clang --version >/dev/null 2>&1 ; then
    fail "Xcode Command Line Tools not found (or license not accepted)." \
        "Install them with:" \
        "    xcode-select --install" \
        "then accept the license with:" \
        "    sudo xcodebuild -license accept"
fi


##### Package manager + the libraries the client links against

BREW_PREFIX=$(brew --prefix 2>/dev/null || true)
PORT_BIN=$(command -v port 2>/dev/null || true)

if [ -n "$BREW_PREFIX" ] ; then
    PKG_MANAGER=brew
elif [ -n "$PORT_BIN" ] ; then
    PKG_MANAGER=port
else
    fail "Neither Homebrew nor MacPorts found." \
        "Install one of:" \
        "    Homebrew:  https://brew.sh" \
        "    MacPorts:  https://www.macports.org/install.php"
fi

if [ "$PKG_MANAGER" = "brew" ] ; then

    MISSING=""
    for formula in gnu-sed libpng sdl12-compat ; do
        brew --prefix "$formula" >/dev/null 2>&1 || MISSING="$MISSING $formula"
    done

    if [ -n "$MISSING" ] ; then
        fail "Missing Homebrew packages:$MISSING" \
            "Install them with:" \
            "    brew install$MISSING"
    fi

else

    MISSING=""
    for p in sdl12-compat libpng ; do
        port installed "$p" 2>/dev/null | grep -q '(active)' || MISSING="$MISSING $p"
    done
    command -v gsed >/dev/null 2>&1 || MISSING="$MISSING gsed"

    if [ -n "$MISSING" ] ; then
        fail "Missing MacPorts ports:$MISSING" \
            "Install them with:" \
            "    sudo port install$MISSING"
    fi

fi


##### A universal (arm64+x86_64) build additionally needs those libraries
##### rebuilt as such -- see installMacPortsDeps.sh.

archs="${MACOSX_ARCHS:-$(uname -m)}"
ARCH_COUNT=$(set -- $archs ; echo $#)

if [ "$ARCH_COUNT" -gt 1 ] ; then

    if [ "$PKG_MANAGER" != "port" ] ; then
        fail "MACOSX_ARCHS requests a universal build, but Homebrew ships no" \
            "universal bottles for sdl12-compat/libpng." \
            "Install MacPorts instead:  https://www.macports.org/install.php"
    fi

    NOT_UNIVERSAL=""
    for lib in libiconv libsdl2 sdl12-compat libpng libjpeg-turbo ; do
        port installed "$lib" 2>/dev/null | grep -q -- '+universal.*(active)' \
            || NOT_UNIVERSAL="$NOT_UNIVERSAL $lib"
    done

    if [ -n "$NOT_UNIVERSAL" ] ; then
        fail "Not built universal yet:$NOT_UNIVERSAL" \
            "Run this first, then re-run this script:" \
            "    build/macOSX/installMacPortsDeps.sh"
    fi

fi


##### Build

echo "--- configuring (MacOSX) ---"
./configure 2

echo "--- compiling ---"
( cd gameSource && make )


##### Package

RELEASE_NAME=${1:-v$(cat ../OneLifeData7/dataVersionNumber.txt)}

echo "--- packaging $RELEASE_NAME ---"
build/makeReleaseFolder "$RELEASE_NAME" 2
