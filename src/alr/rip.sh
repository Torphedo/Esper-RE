#!/bin/bash

# Run this script in the build folder, with the "alr" binary and # the "bin"
# folder full of ALR files in the same folder. It will rip textures from all
# ALR files on all threads using the GNU parallel tool. You might need to
# install parallel first.

# Go into bin/ folder with all the ALR files
cd bin

find . -name "*.alr" | parallel --verbose --max-args 1 ../single_rip.sh {}

