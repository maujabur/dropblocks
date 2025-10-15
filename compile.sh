#!/bin/bash

# Script de compilação e execução do DropBlocks
# Para uso no MSYS2 UCRT64

echo "🚀 Iniciando compilação do DropBlocks..."

# Navegar para o diretório do projeto
cd /c/Users/User/Documents/dropblocks

echo "📁 Diretório atual: $(pwd)"

# Verificar se o arquivo fonte existe
if [ ! -f "dropblocks.cpp" ]; then
    echo "❌ Erro: Arquivo dropblocks.cpp não encontrado!"
    exit 1
fi

echo "🔧 Compilando dropblocks.cpp..."

# Compilar o projeto
g++ dropblocks.cpp -o dropblocks.exe $(sdl2-config --cflags --libs) -O2 -std=c++17

# Verificar se a compilação foi bem-sucedida
if [ $? -eq 0 ]; then
    echo "✅ Compilação concluída com sucesso!"
    echo "🎮 Executando DropBlocks..."
    echo "----------------------------------------"
    
    # Executar o jogo
    ./dropblocks.exe
    
    echo "----------------------------------------"
    echo "🏁 DropBlocks finalizado."
else
    echo "❌ Erro na compilação!"
    exit 1
fi
