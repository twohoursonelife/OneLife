#!/bin/sh

#
# Modification History
#
# 2026-September-16    hobby-dev
# Created.  Builds and packages a macOS client release: configure, make,
# build/makeReleaseFolder, then verifies the packaged binary has every
# arch MACOSX_ARCHS asked for (make only checks file timestamps, so a
# stale single-arch .o can silently drop an arch from the link).  Checks
# build dependencies up front and says what's missing -- never installs
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


##### Verify: see modification history above for why this matters.

GAME_BINARY="build/release/2HOL_$RELEASE_NAME/2HOL_$RELEASE_NAME.app/Contents/MacOS/OneLife"

BUILT_ARCHS=$(lipo -archs "$GAME_BINARY" 2>&1) \
    || fail "lipo couldn't read $GAME_BINARY -- is it a valid Mach-O binary?" "$BUILT_ARCHS"

for arch in $archs ; do
    case " $BUILT_ARCHS " in
        *" $arch "*) ;;
        *) fail "Packaged binary is missing arch '$arch' (built: $BUILT_ARCHS)." \
            "Usually a stale .o from an earlier single-arch build -- clean and retry:" \
            "    find . ../minorGems -name '*.o' -delete" ;;
    esac
done

echo "--- verified $GAME_BINARY is built for: $BUILT_ARCHS ---"
