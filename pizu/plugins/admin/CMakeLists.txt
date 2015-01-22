
cmake_minimum_required (VERSION 2.6)
PROJECT(HTMLMOD)

include_directories ("../../../mcoinclude")
include_directories ("../../../mcosocks")
ADD_LIBRARY(admin_mod SHARED "adminmod.cpp" "main.cpp")

SET(LIBRARY_OUTPUT_PATH "../../../bin/pizu-plugins")
