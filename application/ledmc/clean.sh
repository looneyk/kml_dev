#!/bin/bash
pushd . -n
cd ../../library/quetzal
if ! make clean; then
    exit 1
fi
cd xml
if ! make clean; then
    exit 1
fi
cd ../pi_gpio
if ! make clean; then
    exit 1
fi
popd
make clean
