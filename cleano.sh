#!/bin/bash
##################################

find -name '*.o' | xargs rm-original
find -name '*.save' | xargs rm-original
find -name CMakeCache.txt | xargs rm-original
find -name CMakeFiles | xargs rm-original -r
