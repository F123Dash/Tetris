# Tetris
A modern implementation of the classic Tetris game written in C using SDL2.

## Features

- Classic Tetris gameplay
- Modern UI
- Top 3 high scores system
- Player name input
- Pause functionality
- Score tracking

## Controls

- **A/D**: Move piece left/right
- **S**: Soft drop
- **R**: Rotate piece
- **ESC**: Pause game
- **Space**: Play again (after game over)
- **E**: Exit game
- **M**: Exit to Login Window

## Dependencies

- SDL2
- SDL2_ttf
- C compiler

## Building

Make sure you have SDL2 and SDL2_ttf development libraries installed:

Then compile the game:

```bash
g++ -I src\include -L src\lib -o tetris tetris.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
```
OtherWise save the MakeFile and run it 
```bash
mingw32-make
```

## Running

```bash
./tetris
```
