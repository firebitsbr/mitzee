#!/bin/bash
./cleano.sh

[[ ! $1 ]] && echo "pass arguments" && exit 1

guglegit.sh $1
