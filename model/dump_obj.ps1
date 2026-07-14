
$configuration = 'Debug'

# application list, loop through in build and run ...

function dump_obj {
    $filepath = $args[0]

    $path = "../bin/$configuration/dump_obj.exe"
    & $path $filepath

    if ($LASTEXITCODE -ne 0) {
        echo "Command failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE 
    }
}

dump_obj $args[0]
exit

dump_obj primitives/out
#dump_obj helix_pyramid/out
# 4 ...
#dump_obj menger_sponge/out
# open ...
dump_obj spikosahedron/out
#dump_obj spinward/out
#dump_obj spoctahedron/out
# big ...
#dump_obj trinket_box/out
# half way, but enough to work on ...
#dump_obj wimble/out
## 300 ...
#dump_obj laminaria/out
