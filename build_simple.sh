#!/bin/bash

# ===========================
#   DROPBLOCKS - BUILD SIMPLES
#   Script de compilação simples sem flags complexas
# ===========================

echo "=== DROPBLOCKS SIMPLE BUILD v7.3 ==="
echo "Building version 7.3 - Window Creation Test Only"
echo "Building simple version without complex flags..."

# Verificar se SDL2 está instalado
if ! pkg-config --exists sdl2; then
    echo "ERROR: SDL2 not found. Installing SDL2..."
    sudo apt-get update
    sudo apt-get install -y libsdl2-dev libsdl2-2.0-0
fi

# Flags básicas
CFLAGS="-O2"
CXXFLAGS="-O2 -std=c++17"

# Flags do SDL2
SDL_CFLAGS=$(pkg-config --cflags sdl2)
SDL_LIBS=$(pkg-config --libs sdl2)

# Compilar teste da janela
echo "Compiling window test..."
g++ $CXXFLAGS $SDL_CFLAGS -g -Wall test_window.cpp -o test_window $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Window test build successful: test_window"
else
    echo "ERROR: Window test build failed"
    exit 1
fi

# Compilar versão limpa
echo "Compiling clean version..."
g++ $CXXFLAGS $SDL_CFLAGS -g -Wall dropblocks_clean.cpp -o dropblocks_clean $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Clean version build successful: dropblocks_clean"
else
    echo "ERROR: Clean version build failed"
    exit 1
fi

# Compilar versão debug simples
echo "Compiling debug version..."
g++ $CXXFLAGS $SDL_CFLAGS -g -Wall dropblocks.cpp -o dropblocks_debug $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Debug build successful: dropblocks_debug"
else
    echo "ERROR: Debug build failed"
    exit 1
fi

# Compilar versão de produção
echo "Compiling production version..."
g++ $CXXFLAGS $SDL_CFLAGS -O2 dropblocks.cpp -o dropblocks $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Production build successful: dropblocks"
else
    echo "ERROR: Production build failed"
    exit 1
fi

# Compilar teste de inicialização
echo "Compiling initialization test..."
g++ $CXXFLAGS $SDL_CFLAGS -g -Wall test_init.cpp -o test_init $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Initialization test build successful: test_init"
else
    echo "ERROR: Initialization test build failed"
    exit 1
fi

# Compilar teste de renderização
echo "Compiling render test..."
g++ $CXXFLAGS $SDL_CFLAGS -g -Wall test_render.cpp -o test_render $SDL_LIBS -lm

if [ $? -eq 0 ]; then
    echo "Render test build successful: test_render"
else
    echo "ERROR: Render test build failed"
    exit 1
fi

echo ""
echo "=== BUILD COMPLETE ==="
echo "Files created:"
echo "  - test_init (initialization test)"
echo "  - test_render (render test)"
echo "  - dropblocks_debug (debug version)"
echo "  - dropblocks (production version)"
echo ""
echo "To test:"
echo "  ./test_init        # Test SDL2 initialization"
echo "  ./test_render      # Test basic rendering"
echo "  ./dropblocks_debug # Test full game with debug"
echo "  ./dropblocks       # Test full game"
