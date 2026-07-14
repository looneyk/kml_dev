#!/bin/bash

dir="$1"
name="$2"

configuration=Debug
platform=x64

cmd.exe /c "MSBuild ../build/$dir/$name/$name.vcxproj /t:Clean /p:Configuration=$configuration /p:Platform=$platform"
