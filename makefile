all:
	g++ -I src\include -L src\lib -o tetris tetris.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf