
#/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(udfjj39546284@gmail.com) - getic.net - N/A
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*/

###############################################################################################
##  this header in CMakeLists.tx is mandatory in every project from HERE { << find closing pair
###############################################################################################
cmake_minimum_required(VERSION 2.6)

################################################################################################
# To HERE                                                                   }
################################################################################################
PROJECT(MIKI)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -fexceptions -fpermissive -D__USE_MISC")


ADD_EXECUTABLE(miki config.cpp main.cpp message.cpp watcher.cpp theapp.cpp udpsock.cpp ../mcoinclude/minidb.cpp ../mcoinclude/sslcrypt.cpp)


include_directories (../mcosocks ../mcoinclude)
link_directories(../lib/)

TARGET_LINK_LIBRARIES(miki pthread dl ${CMAKE_CURRENT_LIST_DIR}/../lib/libmcosock.a)

SET(EXECUTABLE_OUTPUT_PATH ../bin)

