CXX = g++
CXXFLAGS = -I/opt/homebrew/include -D_THREAD_SAFE
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

tetris: tetris.c
	$(CXX) $(CXXFLAGS) -o tetris tetris.c $(LDFLAGS)

clean:
	rm -f tetris