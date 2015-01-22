#!/bin/bash

find -name '*.o' | xargs rm $1
find -name '*~' | xargs rm $1
