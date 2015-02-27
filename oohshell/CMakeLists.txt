#/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - getic.net - N/A
#           FOR HOME USE ONLY. For corporate  please contact me
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


PROJECT(oohshell)
add_executable (ooohshell main.cpp argspars.cpp clicontext.cpp tcppipe.cpp ttyptmdev.cpp)
include_directories (../mcosocks ../mcoinclude ../dnsprx_common)
link_directories(../lib/)
TARGET_LINK_LIBRARIES(ooohshell pthread dl ${CMAKE_CURRENT_LIST_DIR}/../lib/libmcosock.a)
SET(EXECUTABLE_OUTPUT_PATH bin)
