
cmake_minimum_required (VERSION 2.6)
PROJECT(PROJECTA)



include_directories ("../mcoinclude")
ADD_LIBRARY(mcosock STATIC "sock.cpp" )
SET(LIBRARY_OUTPUT_PATH ../lib)


