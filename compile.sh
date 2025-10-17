#!/bin/bash

# Script de compilação e execução do DropBlocks
# Para uso no MSYS2 UCRT64

clear

echo "🚀 Building DropBlocks"

# Navegar para o diretório do projeto
cd /c/Users/User/Documents/dropblocks

echo "📁 Diretório atual: $(pwd)"

# Verificar se o arquivo fonte existe
if [ ! -f "dropblocks.cpp" ]; then
    echo "❌ Erro: Arquivo dropblocks.cpp não encontrado!"
    exit 1
fi

echo "🔧 Preparando fontes..."

# Coletar fontes
SRC_LIST="dropblocks.cpp"
if [ -d src ]; then
  SRC_FROM_SRC=$(find src -type f -name '*.cpp' 2>/dev/null | tr '\n' ' ')
  if [ -n "$SRC_FROM_SRC" ]; then
    SRC_LIST="$SRC_LIST $SRC_FROM_SRC"
  fi
fi

echo "🔧 Compilando: $SRC_LIST"
g++ -Iinclude $SRC_LIST -o dropblocks.exe $(sdl2-config --cflags --libs) -O2 -std=c++17

# Verificar se a compilação foi bem-sucedida
if [ $? -eq 0 ]; then
    echo "✅ Compilação concluída com sucesso!"
    echo "🎮 Executando DropBlocks..."
    echo "----------------------------------------"
    
    # Executar o jogo
    #./dropblocks.exe
    DROPBLOCKS_CFG="generic.cfg" DROPBLOCKS_PIECES="tetrominos.pieces" ./dropblocks.exe
    
    echo "----------------------------------------"
    echo "🏁 DropBlocks finalizado."
else
    echo "❌ Erro na compilação!"
    exit 1
fi

# Optional tests target
if [ "$1" = "test" ]; then
  echo "🧪 Running unit tests..."
  if [ -f "tests/run_tests.sh" ]; then
    bash tests/run_tests.sh
  else
    echo "ℹ️  Tests not found (skipping)"
  fi
fi
