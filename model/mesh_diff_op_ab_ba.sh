#!/usr/bin/env bash
#
# Compares the _ab and _ba output of mesh_operations. The two differ only in the order the submeshes are
# given, so every pair should describe the same solid. Run from the model directory.
#
# mesh_diff reports Different for a pair whose faces all match but whose surface names do not. union and
# intersection name the result submesh after the operation, so their surface names never match across a
# pair. Those are counted separately here, and only face differences set the exit status.
#
# Optional argument limits the comparison to models whose name contains it. CONFIGURATION and OPERATIONS
# override the defaults. Only operations that give the same result whichever submesh comes first belong
# in OPERATIONS; difference does not, and the _clipped and _separated variations are excluded by the name
# match below.

set -u

model_name=${1-}
configuration=${CONFIGURATION:-Debug}
operations=${OPERATIONS:-"identity union intersection symmetric_difference"}

dir_out=mesh_operations/out
path_mesh_diff=../bin/$configuration/mesh_diff.exe

if [ ! -f "$path_mesh_diff" ]; then
    echo "mesh_diff not found at $path_mesh_diff"
    exit 1
fi

dir_temp=$(mktemp -d)
trap 'rm -rf "$dir_temp"' EXIT
mkdir -p "$dir_temp/ab" "$dir_temp/ba"

# mesh_diff pairs files by name across the two directories, so each pair is copied in under a common name
pair_count=0
for operation in $operations; do
    for path_ab in "$dir_out"/*_"${operation}"_ab.obj; do
        [ -e "$path_ab" ] || continue

        name=$(basename "$path_ab" .obj)
        model=${name%_${operation}_ab}

        case "$model" in
            *"$model_name"*) ;;
            *) continue ;;
        esac

        path_ba="$dir_out/${model}_${operation}_ba.obj"
        [ -f "$path_ba" ] || continue

        cp "$path_ab" "$dir_temp/ab/${model}_${operation}.obj"
        cp "$path_ba" "$dir_temp/ba/${model}_${operation}.obj"
        pair_count=$((pair_count + 1))
    done
done

if [ "$pair_count" -eq 0 ]; then
    echo "No _ab/_ba pairs found"
    exit 1
fi

output=$("$path_mesh_diff" "$dir_temp/ab" "$dir_temp/ba" || true)

names_only='0 reversed, 0 different, 0 unmatched, 0 extra'

identical=$(printf '%s\n' "$output" | grep -E "$(printf '\t')Identical" || true)
equivalent=$(printf '%s\n' "$output" | grep -E "$(printf '\t')Equivalent" || true)
different=$(printf '%s\n' "$output" | grep -E "$(printf '\t')Different" || true)
naming=$(printf '%s\n' "$different" | grep -F "$names_only" || true)
differing=$(printf '%s\n' "$different" | grep -vF "$names_only" || true)

count() { [ -z "$1" ] && echo 0 || printf '%s\n' "$1" | wc -l | tr -d ' '; }

echo "$pair_count _ab/_ba pairs compared"
echo "  identical             $(count "$identical")"
echo "  equivalent            $(count "$equivalent")"
echo "  surface names only    $(count "$naming")"
echo "  face differences      $(count "$differing")"

if [ -n "$equivalent" ]; then
    echo
    echo "Equivalent, faces matched but differing in triangulation or vertex order:"
    printf '%s\n' "$equivalent" | sed 's/^/  /'
fi

if [ -n "$differing" ]; then
    echo
    echo "Face differences:"
    printf '%s\n' "$differing" | sed 's/^/  /'
    exit 1
fi

exit 0
