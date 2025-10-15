# DropBlocks 🎮

A modern, customizable falling blocks game inspired by Tetris®, built with C++ and SDL2. Features advanced visual effects, customizable themes, and multiple piece sets.

![DropBlocks Screenshot](dropblocks-screenshot_2025-10-03_02-20-51.bmp)

## ✨ Features

- **Modern Visual Effects**: Gaussian sweep effects, scanlines, and smooth animations
- **Customizable Themes**: Full color customization through configuration files
- **Multiple Piece Sets**: Support for different piece configurations and randomizers
- **Advanced Audio**: Sound effects with volume control and ambient sounds
- **SRS Rotation System**: Super Rotation System with proper kick mechanics
- **Screenshot Support**: Press F12 to capture screenshots
- **Cross-Platform**: Works on Windows, Linux, and macOS

## 🎮 Controls

| Key | Action |
|-----|--------|
| `←` `→` | Move piece left/right |
| `↓` | Soft drop |
| `Z` / `↑` | Rotate counter-clockwise |
| `X` | Rotate clockwise |
| `SPACE` | Hard drop |
| `P` | Pause |
| `ENTER` | Restart (after Game Over) |
| `ESC` | Quit |
| `F12` | Screenshot |

## 🚀 Quick Start

### Prerequisites

- **C++17 compatible compiler** (GCC, Clang, or MSVC)
- **SDL2 development libraries**
- **CMake** (optional, for easy building)

### Building

#### Windows (MSYS2 - Recommended)

1. Install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 terminal and install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-SDL2
   ```
3. Compile:
   ```bash
   g++ dropblocks.cpp -o dropblocks.exe `sdl2-config --cflags --libs` -O2 -std=c++17
   ```

#### Linux

```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev g++

# Compile
g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2 -std=c++17
```

#### macOS

```bash
# Install SDL2 via Homebrew
brew install sdl2

# Compile
g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2 -std=c++17
```

### Running

```bash
./dropblocks
```

## 🎨 Configuration

DropBlocks is highly customizable through configuration files:

### Basic Configuration

Create a `dropblocks.cfg` file to customize:

```ini
# Visual Effects
ENABLE_BANNER_SWEEP = true
ENABLE_GLOBAL_SWEEP = true
SCANLINE_ALPHA = 20

# Colors
BG = #08080C
BOARD_EMPTY = #1C1C24
PANEL_FILL = #181820

# Audio
AUDIO_MASTER_VOLUME = 1.0
AUDIO_SFX_VOLUME = 0.75
```

### Piece Sets

Define custom piece sets in `.pieces` files:

```ini
[SET]
NAME = My Custom Set
PREVIEWGRID = 4

[PIECE.I]
NAME = I-Piece
COLOR = #00FFFF
ROTATIONS = auto
BASE = (0,0);(1,0);(2,0);(3,0)
KICKS.CW = (0,0);(-2,0);(1,0);(-2,-1);(1,2)
```

## 📁 Project Structure

```
dropblocks/
├── dropblocks.cpp          # Main game source code
├── README.md               # This file
├── *.cfg                   # Configuration files
├── *.pieces               # Piece set definitions
└── *.bmp                  # Screenshots
```

### Configuration Files

- `tetris_original.cfg` - Classic Tetris configuration
- `teste_sweep_*.cfg` - Various sweep effect configurations
- `teste_scanlines.cfg` - Scanline effect configuration
- `audio_*.cfg` - Audio configuration examples

### Piece Sets

- `tetris_original.pieces` - Standard Tetris pieces
- `tetrominos.pieces` - Alternative piece set
- `test_*.pieces` - Test piece configurations

## 🎯 Game Features

### Visual Effects

- **Banner Sweep**: Animated sweep effect across the title banner
- **Global Sweep**: Screen-wide lighting effect
- **Scanlines**: Retro CRT-style scanline effect
- **Rounded Panels**: Modern UI with rounded corners

### Audio System

- **Master Volume Control**: Overall audio level
- **SFX Volume**: Sound effects volume
- **Ambient Sounds**: Background audio
- **Combo Sounds**: Special sounds for line clears
- **Level Up Sounds**: Audio feedback for level progression

### Game Mechanics

- **SRS Rotation**: Super Rotation System with proper wall kicks
- **Bag Randomizer**: 7-bag randomizer for fair piece distribution
- **Level Progression**: Increasing speed and difficulty
- **Line Clear Scoring**: Points for single, double, triple, and Tetris clears

## 🔧 Advanced Configuration

### Sweep Effects

Customize the visual sweep effects:

```ini
# Banner Sweep
SWEEP_SPEED_PXPS = 15.0
SWEEP_BAND_H_S = 30
SWEEP_ALPHA_MAX = 100
SWEEP_SOFTNESS = 0.7

# Global Sweep
SWEEP_G_SPEED_PXPS = 20.0
SWEEP_G_BAND_H_PX = 100
SWEEP_G_ALPHA_MAX = 50
SWEEP_G_SOFTNESS = 0.9
```

### Randomizer Options

```ini
# Simple randomizer
RAND_TYPE = simple

# Bag randomizer (recommended)
RAND_TYPE = bag
RAND_BAG_SIZE = 7
```

## 🐛 Troubleshooting

### Common Issues

**"g++ not found"**
- Install a C++ compiler (GCC, Clang, or MSVC)
- Add compiler to your PATH

**"SDL2 not found"**
- Install SDL2 development libraries
- Ensure `sdl2-config` is in your PATH

**Compilation errors**
- Verify C++17 support
- Check all dependencies are installed
- Try a different compiler

### Performance Issues

- Reduce `SCANLINE_ALPHA` for better performance
- Disable global sweep effects if needed
- Lower audio quality settings

## 🤝 Contributing

Contributions are welcome! Please feel free to submit:

- Bug reports
- Feature requests
- Pull requests
- New piece sets
- Theme configurations

## 📄 License

This project is for educational purposes. Tetris® is a registered trademark of The Tetris Company.

## 🙏 Acknowledgments

- **SDL2** - Cross-platform multimedia library
- **Tetris Company** - Original Tetris concept
- **Community** - For feedback and contributions

---

**Enjoy playing DropBlocks!** 🎮✨