#!/bin/bash

# ===========================
#   DROPBLOCKS - BUILD SCRIPT PARA RASPBERRY PI
#   Script de compilação otimizado para Raspberry Pi
# ===========================

echo "=== DROPBLOCKS BUILD SCRIPT FOR RASPBERRY PI v5.0 ==="
echo "Building version 5.0 - Raspberry Pi Segmentation Fault Fix"
echo "Building optimized version for ARM architecture..."

# Verificar se SDL2 está instalado
if ! pkg-config --exists sdl2; then
    echo "ERROR: SDL2 not found. Installing SDL2..."
    sudo apt-get update
    sudo apt-get install -y libsdl2-dev libsdl2-2.0-0
fi

# Verificar versão do SDL2
echo "SDL2 version: $(pkg-config --modversion sdl2)"

# Flags de compilação otimizadas para Raspberry Pi
CFLAGS="-O2 -march=armv7-a -mfpu=neon -mfloat-abi=hard -ffast-math"
CXXFLAGS="-O2 -march=armv7-a -mfpu=neon -mfloat-abi=hard -ffast-math -std=c++17"

# Flags do SDL2
SDL_CFLAGS=$(pkg-config --cflags sdl2)
SDL_LIBS=$(pkg-config --libs sdl2)

# Compilar versão debug simples
echo "Compiling simple debug version..."

g++ $CXXFLAGS $SDL_CFLAGS \
    -DDEBUG \
    -g \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    dropblocks.cpp \
    -o dropblocks_debug \
    $SDL_LIBS \
    -lm

if [ $? -eq 0 ]; then
    echo "Debug build successful: dropblocks_debug"
else
    echo "ERROR: Debug build failed"
    exit 1
fi

# Compilar teste de inicialização
echo "Compiling initialization test..."

g++ $CXXFLAGS $SDL_CFLAGS \
    -g \
    -Wall \
    -Wextra \
    test_init.cpp \
    -o test_init \
    $SDL_LIBS \
    -lm

if [ $? -eq 0 ]; then
    echo "Initialization test build successful: test_init"
else
    echo "ERROR: Initialization test build failed"
    exit 1
fi

# Compilar versão otimizada para produção
echo "Compiling optimized production version..."

g++ $CXXFLAGS $SDL_CFLAGS \
    -DNDEBUG \
    -O3 \
    -march=armv7-a \
    -mfpu=neon \
    -mfloat-abi=hard \
    -ffast-math \
    -funroll-loops \
    -fomit-frame-pointer \
    dropblocks.cpp \
    -o dropblocks \
    $SDL_LIBS \
    -lm

if [ $? -eq 0 ]; then
    echo "Production build successful: dropblocks"
else
    echo "ERROR: Production build failed"
    exit 1
fi

# Verificar se os executáveis foram criados
if [ -f "dropblocks" ] && [ -f "dropblocks_debug" ] && [ -f "test_init" ]; then
    echo ""
    echo "=== BUILD COMPLETE ==="
    echo "Files created:"
    echo "  - test_init (initialization test)"
    echo "  - dropblocks_debug (debug version with safety checks)"
    echo "  - dropblocks (production version)"
    echo ""
    echo "To test initialization first:"
    echo "  ./test_init           # Test SDL2 initialization step by step"
    echo ""
    echo "To run the game:"
    echo "  ./dropblocks_debug    # Debug version with safety checks"
    echo "  ./dropblocks          # Production version"
    echo ""
    echo "If you get segmentation fault, run:"
    echo "  gdb ./test_init       # Debug initialization test"
    echo "  gdb ./dropblocks_debug # Debug full game"
    echo "  (gdb) run"
    echo "  (gdb) bt"
    echo ""
else
    echo "ERROR: Build files not found"
    exit 1
fi
