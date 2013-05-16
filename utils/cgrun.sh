#!/usr/bin/env bash

cgexec -g blkio,cpuset:perftest42 "$@"

