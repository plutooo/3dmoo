#!/bin/bash
./build.sh && gdb --args ./3dmuu $1
exit $?
