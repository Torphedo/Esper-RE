#!/bin/bash

# Remove the extension from the file extension.
filename="${1%.*}"

mkdir -p $filename

cp alr $filename

cd $filename

./alr --dump "../$1"

# Delete temp files.
rm alr

rm -rf streams

