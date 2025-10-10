#!/bin/bash

# ===========================
#   DROPBLOCKS - DEBUG SCRIPT PARA RASPBERRY PI
#   Script para debugging de segmentation fault
# ===========================

echo "=== DROPBLOCKS DEBUG SCRIPT FOR RASPBERRY PI ==="

# Verificar se o executável existe
if [ ! -f "dropblocks_debug" ]; then
    echo "ERROR: dropblocks_debug not found. Run build_raspberry_pi.sh first."
    exit 1
fi

# Verificar dependências
echo "Checking dependencies..."
ldd dropblocks_debug

echo ""
echo "=== RUNNING WITH VALGRIND (if available) ==="
if command -v valgrind &> /dev/null; then
    echo "Running with Valgrind memory checker..."
    valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./dropblocks_debug
else
    echo "Valgrind not available. Installing..."
    sudo apt-get update
    sudo apt-get install -y valgrind
    valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./dropblocks_debug
fi

echo ""
echo "=== RUNNING WITH GDB ==="
echo "Starting GDB debugger..."
echo "Commands to use in GDB:"
echo "  (gdb) run"
echo "  (gdb) bt          # Show backtrace when it crashes"
echo "  (gdb) info registers"
echo "  (gdb) x/20x \$rsp  # Examine stack"
echo "  (gdb) quit"
echo ""

gdb ./dropblocks_debug

