#!/bin/sh
# Receives in stdin a file in DIMACS challenge 9 format
# Outputs a file in edgelist format, separated by tabs
#

grep "^a" - | cut -d ' ' -f 2-4 | tr ' ' '\t'
