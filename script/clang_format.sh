#!/usr/bin/env bash

set -u -e -o pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

format_pattern () {
    local PREFIX
    local EXTENSION

    PREFIX="$1"
    EXTENSION="$2"

    find "$ROOT_DIR/$PREFIX" -type f -name "*.$EXTENSION" -exec sh -c '
        echo "$1"
        clang-format -i -style=file "$1"
    ' sh {} \;
}

main() {
    format_pattern "src" "cpp"
    format_pattern "include" "hpp"
    format_pattern "test" "cpp"
    format_pattern "test" "hpp"
}

main
