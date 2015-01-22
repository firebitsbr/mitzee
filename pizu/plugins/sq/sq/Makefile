
SRC =	sqstdlib/sqstdaux.cpp \
		sqstdlib/sqstdblob.cpp \
		sqstdlib/sqstdio.cpp \
		sqstdlib/sqstdmath.cpp \
		sqstdlib/sqstdrex.cpp \
		sqstdlib/sqstdstream.cpp \
		sqstdlib/sqstdstring.cpp \
		sqstdlib/sqstdsystem.cpp \
		squirrel/sqapi.cpp \
		squirrel/sqbaselib.cpp \
		squirrel/sqclass.cpp \
		squirrel/sqcompiler.cpp \
		squirrel/sqdebug.cpp \
		squirrel/sqfuncstate.cpp \
		squirrel/sqlexer.cpp \
		squirrel/sqmem.cpp \
		squirrel/sqobject.cpp \
		squirrel/sqstate.cpp \
		squirrel/sqtable.cpp \
		squirrel/sqvm.cpp
		
 
OBJ = $(SRC:.cpp=.o)
OUT = ../../../../lib/libsq.a
INCLUDES = -Iinclude/
CCFLAGS = -fPIC -g -fexceptions -Wall -D__LINUX_TARGET -DXX_DEBUG_DUMP
CCC = g++

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)



