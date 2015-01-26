#!/bin/bash
##################################

find -name '*.o' | xargs rm
find -name '*.save' | xargs rm
find -name CMakeCache.txt | xargs rm
find -name CMakeFiles | xargs rm -r
