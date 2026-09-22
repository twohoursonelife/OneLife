#!/bin/sh

#
# Modification History
#
# 2026-September-16
# Created.  Rebuilds the MacPorts libraries the macOS client needs
# (sdl12-compat, libsdl2, libpng, libjpeg-turbo, libiconv) as universal
# (arm64+x86_64) builds targeting MACOSX_DEPLOYMENT_TARGET (default 11.0).
# Plain `port install` won't do this -- macosx_deployment_target must be
# passed as a per-port option (the env var alone is ignored), so this
# forces a clean, from-source rebuild with it set explicitly.
#
# Requires MacPorts and an accepted Xcode license:
#     sudo xcodebuild -license accept
#
# Usage:
#     ./installMacPortsDeps.sh
#     MACOSX_DEPLOYMENT_TARGET=10.13 ./installMacPortsDeps.sh
#

set -e

MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"


if ! command -v port >/dev/null 2>&1 ; then
    if command -v brew >/dev/null 2>&1 ; then
        echo "$0: Homebrew found, but no MacPorts ('port' not on PATH)." >&2
        echo "This script only automates the MacPorts route.  For a Homebrew" >&2
        echo "build (arm64-only -- Homebrew ships no universal bottles), follow" >&2
        echo ".github/workflows/build-release.yml's build-mac job instead:" >&2
        echo "    brew install rsync wget imagemagick gnu-sed libpng sdl12-compat" >&2
        echo "    cd OneLife && ./configure 2 && cd gameSource && make" >&2
    else
        echo "$0: no package manager found (neither 'port' nor 'brew' on PATH)." >&2
        echo "Install MacPorts from https://www.macports.org/install.php first." >&2
    fi
    exit 1
fi

if ! /usr/bin/xcrun clang --version >/dev/null 2>&1 ; then
    echo "$0: Xcode license not accepted yet." >&2
    echo "Run this first, then re-run this script:" >&2
    echo "    sudo xcodebuild -license accept" >&2
    exit 1
fi


# -N: run unattended, answering every prompt as yes.
echo "--- updating MacPorts' port definitions ---"
sudo port -N selfupdate


echo
echo "Building for MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET (universal: arm64 + x86_64)"
echo


# Build-time tools only, not linked into the game -- a native-arch
# install is fine.
echo "--- installing convert (ImageMagick) and gsed ---"
sudo port -N install ImageMagick gsed


# Linked by the game and bundled into the .app -- see Makefile.MacOSX.
LIBS="libiconv libsdl2 sdl12-compat libpng libjpeg-turbo"

# True if $1 is already +universal, active, and built for arm64+x86_64 at
# MACOSX_DEPLOYMENT_TARGET -- lets re-runs skip an already-done library.
lib_is_up_to_date() {
    port installed "$1" 2>/dev/null | grep -q -- '+universal.*(active)' || return 1

    dylib=$(port contents "$1" 2>/dev/null | grep -m1 '\.dylib$' | xargs)
    [ -n "$dylib" ] && [ -e "$dylib" ] || return 1

    archs=" $(lipo -archs "$dylib" 2>/dev/null) "
    case "$archs" in *" arm64 "*) ;; *) return 1 ;; esac
    case "$archs" in *" x86_64 "*) ;; *) return 1 ;; esac

    minos=$(otool -l "$dylib" 2>/dev/null | awk '
        /cmd LC_BUILD_VERSION/      { want="minos" }
        /cmd LC_VERSION_MIN_MACOSX/ { want="version" }
        want && $1 == want { print $2; want="" }
    ' | sort -u)
    [ "$minos" = "$MACOSX_DEPLOYMENT_TARGET" ]
}

TO_BUILD=""
for lib in $LIBS ; do
    if lib_is_up_to_date "$lib" ; then
        echo "--- $lib already universal, target $MACOSX_DEPLOYMENT_TARGET -- skipping ---"
    else
        TO_BUILD="$TO_BUILD $lib"
    fi
done

if [ -n "$TO_BUILD" ] ; then

    echo
    echo "--- cleaning previous builds of:$TO_BUILD ---"
    # Guards against a stale partial build from an interrupted earlier run.
    sudo port -N clean $TO_BUILD

    echo
    echo "--- rebuilding from source, universal, target $MACOSX_DEPLOYMENT_TARGET:$TO_BUILD ---"
    echo "(this compiles each library twice -- arm64 and x86_64 -- and can take a while)"
    # --force/--enforce-variants: apply +universal even though nothing looks
    # outdated.  --no-rev-upgrade: skip the post-upgrade link scan (a no-op
    # for a universal rebuild).  -n: don't cascade into upgrading unrelated
    # build-time deps.  One port per invocation: MacPorts scopes a trailing
    # variant/option to just the portname right before it.
    for lib in $TO_BUILD ; do
        sudo port -N -n -s upgrade --force --enforce-variants --no-rev-upgrade \
            "$lib" +universal macosx_deployment_target=$MACOSX_DEPLOYMENT_TARGET
    done

else
    echo
    echo "--- all libraries already universal, target $MACOSX_DEPLOYMENT_TARGET -- nothing to build ---"
fi


echo
echo "Done.  Spot-check with, e.g.:"
echo "  otool -l /opt/local/lib/libpng16.16.dylib | grep -A3 LC_BUILD_VERSION"
echo "  lipo -info /opt/local/lib/libpng16.16.dylib"
