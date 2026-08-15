#!/bin/bash

# This script is the main entry point for calling rpl2state.sh
# in parallel, 8 times with different partition IDs.

for i in {0..7}; do
    ./rpl2state.sh "$i" &
done

wait
