#!/bin/bash

find . -name "*.alr" | parallel --verbose --max-args 1 ./single_rip.sh {}
