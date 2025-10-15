# DropBlocks - Instruções para Windows

## Compilação e Execução

### 1. **Pré-requisitos**

- **MinGW-w64** ou **MSYS2** com g++
- **SDL2** development libraries
- **Git** (opcional, para clonar o repositório)

### 2. **Instalação do SDL2**

#### Opção A: MSYS2 (Recomendado)
```bash
# Instalar MSYS2 do site oficial: https://www.msys2.org/
# Abrir MSYS2 MINGW64 terminal e executar:
pacman -S mingw-w64-x86_64-SDL2
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-gdb
```

#### Opção B: MinGW-w64
1. Baixar MinGW-w64 do site oficial
2. Baixar SDL2 development libraries de https://www.libsdl.org/download-2.0.php
3. Extrair SDL2 para uma pasta (ex: `C:\SDL2`)
4. Adicionar `C:\SDL2\i686-w64-mingw32\bin` ao PATH

### 3. **Compilação**

#### Método 1: Script Automático (Recomendado)
```cmd
build_windows.bat
```

#### Método 2: Compilação Manual
```cmd
g++ dropblocks.cpp -o dropblocks.exe `sdl2-config --cflags --libs` -O2
```

### 4. **Execução**

#### Teste de Inicialização
```cmd
test_init.exe
```

#### Jogo (Versão Debug)
```cmd
dropblocks_debug.exe
```

#### Jogo (Versão Otimizada)
```cmd
dropblocks.exe
```

### 5. **Debugging**

#### Script de Debug
```cmd
debug_windows.bat
```

#### Debug Manual com GDB
```cmd
gdb dropblocks_debug.exe
(gdb) run
(gdb) bt
```

## Controles

- **← →** : Mover peça esquerda/direita
- **↓** : Soft drop
- **Z/↑** : Rotacionar anti-horário
- **X** : Rotacionar horário
- **ESPAÇO** : Hard drop
- **P** : Pausar
- **ENTER** : Reiniciar (após Game Over)
- **ESC** : Sair
- **F12** : Screenshot

## Configuração

O jogo carrega configurações dos seguintes arquivos (em ordem):
1. `default.cfg`
2. `dropblocks.cfg`
3. `raspberry_pi.cfg` (se disponível)

## Solução de Problemas

### Segmentation Fault
1. Execute `test_init.exe` primeiro
2. Se falhar, execute `debug_windows.bat`
3. Verifique se SDL2 está instalado corretamente

### Janela não abre em tela cheia
- O jogo tenta fullscreen primeiro, depois janela normal
- Verifique se seu display suporta o modo fullscreen

### Problemas de Áudio
- O jogo continua funcionando sem áudio se houver problemas
- Verifique se os drivers de áudio estão funcionando

## Arquivos Criados

- `dropblocks.exe` - Versão otimizada
- `dropblocks_debug.exe` - Versão com debug
- `test_init.exe` - Teste de inicialização
- `dropblocks-screenshot_*.bmp` - Screenshots (F12)

## Suporte

Se encontrar problemas:
1. Execute `test_init.exe` e reporte a saída
2. Execute `debug_windows.bat` e reporte a saída
3. Inclua informações sobre seu sistema (Windows version, MinGW version, SDL2 version)
