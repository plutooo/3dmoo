#!/bin/bash
./build.sh && gdb --args ./3dmuu $@
exit $?
