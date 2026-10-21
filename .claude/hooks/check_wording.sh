#!/usr/bin/env bash
# Runs check_wording.py with the first Python 3 that works. On Windows, python3 can be the Microsoft Store stub, which
# fails the version check and is skipped
directory="$(dirname "$0")"
for interpreter in python3 python; do
    if "$interpreter" -c "import sys; sys.exit(0 if sys.version_info >= (3, 8) else 1)" > /dev/null 2>&1; then
        exec "$interpreter" "$directory/check_wording.py"
    fi
done
exit 0
