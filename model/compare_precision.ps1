
$configuration = 'Debug'

# application list, loop through in build and run ...

#function build {
#    param (
#        $application
#    )
#
#    cd $application
#    echo $application
#    
#    $path = "../../out/$platform/$configuration/$application.exe"
#    if (Test-Path $path) {
#        rm $path
#    }
#    #rm ../../out/$platform/$configuration/$application.exe
#
#    # MSBuild /p:Platform=$platform /p:Configuration=$confguration /nologo /verbosity:m
#    & MSBuild /nologo /verbosity:m
#    cd ..
#}
#
#function run {
#    param (
#        $application
#    )
#
#    cd $application
#    & ../../out/$platform/$configuration/$application.exe
#    cd ..
#}

function compare_precision {
    param (
        $application
    )

    echo $application

    $path = "../bin/$configuration/compare_precision_obj.exe"

    $reference_path = "./$application/reference"
    $out_path = "./$application/out"

    $filenames = Get-ChildItem $reference_path
    foreach($filename in $filenames) {
        echo "$filename"
        $lhs = "./$application/reference/$filename"
        $rhs = "./$application/out/$filename"
        & $path $lhs $rhs

        if ($LASTEXITCODE -ne 0) {
            echo "Command failed with exit code $LASTEXITCODE"
            exit $LASTEXITCODE 
        }
    }
}

compare_precision -application primitives_test
compare_precision -application helix_cone_test
#compare_precision -application menger_sponge
compare_precision -application spikosahedron
#compare_precision -application spinward
#compare_precision -application spoctahedron
compare_precision -application trinket_box
#compare_precision -application wimble
#compare_precision -application laminaria
