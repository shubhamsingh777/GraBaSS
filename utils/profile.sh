#!/usr/bin/env bash

# config
PFILE=test.profile

# cleanup
rm -f $PFILE
rm -f *.db *.txt

# run
./cgrun.sh env LD_PRELOAD=/usr/lib/libprofiler.so.0 CPUPROFILE=$PFILE ./hugeDim &
child=$!
trap "kill -INT $child" INT
wait $child

