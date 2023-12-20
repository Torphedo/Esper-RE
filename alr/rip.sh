#!/bin/bash

# Run this in the "bin/" folder inside of the build output folder, after
# copying the "alr" binary file into the same folder. See single_rip.sh for
# more details.
find . -name "*.alr" | parallel --verbose --max-args 1 ./single_rip.sh {}

