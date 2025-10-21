#!/bin/bash
pushd . -n
cd ../../library/quetzal
if ! make; then
    exit 1
fi
cd xml
if ! make; then
    exit 1
fi
cd ../pi_gpio
if ! make; then
    exit 1
fi
popd
make
