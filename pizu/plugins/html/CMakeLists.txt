
cmake_minimum_required (VERSION 2.6)
PROJECT(HTMLMOD)

include_directories ("../../../mcoinclude")
include_directories ("../../../mcosocks")
ADD_LIBRARY(html_mod SHARED "htmlmod.cpp" "main.cpp")
SET(LIBRARY_OUTPUT_PATH "../../../bin/pizu-plugins")
