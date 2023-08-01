CXXFLAGS = -I "C:\C++ libs\SDL2-2.28.1\x86_64-w64-mingw32\include" -L "C:\C++ libs\SDL2-2.28.1\x86_64-w64-mingw32\lib"
LIBS = -lSDL2 -lSDL2main

all:
	g++ main.c -o chip8.exe ${CXXFLAGS} ${LIBS}