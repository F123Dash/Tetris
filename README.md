# Modern Tetris

A modern implementation of the classic Tetris game written in C using SDL2.

## Features

- Classic Tetris gameplay
- Modern UI with glow effects
- Top 3 high scores system
- Player name input
- Pause functionality
- Score tracking
- Medal system for high scores (🥇, 🥈, 🥉)

## Controls

- **A/D**: Move piece left/right
- **S**: Soft drop
- **R**: Rotate piece
- **P**: Pause game
- **Space**: Play again (after game over)
- **ESC**: Exit game

## Dependencies

- SDL2
- SDL2_ttf
- C compiler (gcc/clang)

## Building

Make sure you have SDL2 and SDL2_ttf development libraries installed:

```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-ttf-dev

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel

# Arch Linux
sudo pacman -S sdl2 sdl2_ttf
```

Then compile the game:

```bash
gcc -o tetris tetris.c -I src/include -L src/lib $(pkg-config --cflags --libs sdl2 SDL2_ttf)
```

## Running

```bash
./tetris
```

## License

MIT License - feel free to use this code for any purpose.