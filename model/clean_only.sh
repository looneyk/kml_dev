#!/bin/bash

dir="$1"
name="$2"
configuration="${3:-Debug}"
buildroot="${4:-../build}"

platform=x64

cmd.exe /c "MSBuild $buildroot/$dir/$name/$name.vcxproj /t:Clean /p:Configuration=$configuration /p:Platform=$platform"
