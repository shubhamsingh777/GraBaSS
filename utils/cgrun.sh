#!/usr/bin/env bash

cgexec -g "blkio,cpuset,memory:perftest42$USER" "$@"

