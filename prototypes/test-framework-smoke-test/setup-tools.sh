#!/usr/bin/env bash
# PROTOTYPE (issue #18): fetch the toolchain, Renode and pinned snitch. No sudo needed.
set -euo pipefail
TOOLS="${TOOLS:-$HOME/proto-tools}"
SNITCH_COMMIT=c29ce5b465fb02250afb0cb35338488c0e6b1534 # master, 2026-10-07 (v1.3.2-38)
mkdir -p "$TOOLS" && cd "$TOOLS"

if [ ! -d arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi ]; then
    curl -fsSL -o arm.tar.xz 'https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz'
    tar xf arm.tar.xz && rm arm.tar.xz
fi
if [ ! -x renode/renode ]; then
    curl -fsSL -o renode.tar.gz 'https://builds.renode.io/renode-latest.linux-portable-dotnet.tar.gz'
    mkdir -p renode && tar xf renode.tar.gz -C renode --strip-components=1 && rm renode.tar.gz
fi
if [ ! -d snitch ]; then git clone -q https://github.com/snitch-org/snitch.git; fi
git -C snitch checkout -q "$SNITCH_COMMIT"
echo "tools ready in $TOOLS"
