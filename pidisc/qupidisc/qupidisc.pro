#-------------------------------------------------
#
# Project created by QtCreator 2014-09-22T14:15:28
#
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
#
#-------------------------------------------------

QT       += core gui

CXXFLAGS += -static

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qupidisc
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
    udpsocks.cpp

HEADERS  += dialog.h \
    udpsocks.h

FORMS    += dialog.ui
