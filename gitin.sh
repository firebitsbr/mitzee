#!/bin/bash
./cleano.sh

[[ ! $1 ]] && echo "pass arguments" && exit 1

pushd buflea  
guglegit.sh $1
popd

pushd mariuz  
guglegit.sh $1
popd

pushd  mcoinclude  
guglegit.sh $1
popd


pushd mcosocks  
guglegit.sh $1
popd


###########
pushd pizu  
guglegit.sh $1
popd


pushd socklib
guglegit.sh $1
popd
