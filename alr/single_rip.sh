#!/bin/bash

# Remove the extension from the file extension.
filename="${1%.*}"

# Grab alr binary from main build folder & put it into new output folder for
# each ALR.
mkdir -p $filename
cp ../alr $filename
cd $filename

# Rip textures.
pwd
./alr --dump "../$1"

# Delete temp copy of alr binary
rm alr

# Script is for texture ripping, so the chunk streams are unnecessary
rm -rf streams

