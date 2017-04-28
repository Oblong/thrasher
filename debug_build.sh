#!/usr/bin/env bash

set -ex
mkdir build_debug_clang
export CXX="clang++"
export CXXFLAGS="-Wextra -Wfatal-errors -ferror-limit=5"
meson build_debug_clang --buildtype=debug -Db_sanitize=address
