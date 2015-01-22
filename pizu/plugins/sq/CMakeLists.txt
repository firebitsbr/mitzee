
cmake_minimum_required (VERSION 2.6)
PROJECT(SQMOD)




include_directories ("../../../mcoinclude" "../../../mcosocks" "sq/include" "sq/squirrel" "sq/sqrat/include" "sq/sqrat")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x -fexceptions")
ADD_LIBRARY(sq_mod SHARED "globalfoos.cpp" "main.cpp" "sqmod.cpp" "sqwrap.cpp")
TARGET_LINK_LIBRARIES(sq_mod  ${CMAKE_CURRENT_LIST_DIR}/../../../lib/libsq.a)
SET(LIBRARY_OUTPUT_PATH "../../../bin/pizu-plugins")
