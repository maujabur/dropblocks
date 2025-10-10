@echo off
REM ===========================
REM   DROPBLOCKS - BUILD SCRIPT PARA WINDOWS
REM   Script de compilação para Windows
REM ===========================

echo === DROPBLOCKS BUILD SCRIPT FOR WINDOWS ===
echo Building optimized version for Windows...

REM Verificar se SDL2 está disponível
where sdl2-config >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: SDL2 not found. Please install SDL2 development libraries.
    echo Download from: https://www.libsdl.org/download-2.0.php
    pause
    exit /b 1
)

REM Verificar versão do SDL2
echo SDL2 version:
sdl2-config --version

REM Flags de compilação otimizadas para Windows
set CFLAGS=-O2 -march=native -ffast-math
set CXXFLAGS=-O2 -march=native -ffast-math -std=c++17

REM Flags do SDL2
for /f "tokens=*" %%i in ('sdl2-config --cflags') do set SDL_CFLAGS=%%i
for /f "tokens=*" %%i in ('sdl2-config --libs') do set SDL_LIBS=%%i

REM Compilar teste de inicialização
echo Compiling initialization test...
g++ %CXXFLAGS% %SDL_CFLAGS% -g -Wall -Wextra test_init.cpp -o test_init.exe %SDL_LIBS% -lm
if %errorlevel% neq 0 (
    echo ERROR: Initialization test build failed
    pause
    exit /b 1
)
echo Initialization test build successful: test_init.exe

REM Compilar versão debug
echo Compiling debug version...
g++ %CXXFLAGS% %SDL_CFLAGS% -DDEBUG -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable dropblocks.cpp -o dropblocks_debug.exe %SDL_LIBS% -lm
if %errorlevel% neq 0 (
    echo ERROR: Debug build failed
    pause
    exit /b 1
)
echo Debug build successful: dropblocks_debug.exe

REM Compilar versão otimizada para produção
echo Compiling optimized production version...
g++ %CXXFLAGS% %SDL_CFLAGS% -DNDEBUG -O3 -march=native -ffast-math -funroll-loops -fomit-frame-pointer dropblocks.cpp -o dropblocks.exe %SDL_LIBS% -lm
if %errorlevel% neq 0 (
    echo ERROR: Production build failed
    pause
    exit /b 1
)
echo Production build successful: dropblocks.exe

REM Verificar se os executáveis foram criados
if exist "dropblocks.exe" if exist "dropblocks_debug.exe" if exist "test_init.exe" (
    echo.
    echo === BUILD COMPLETE ===
    echo Files created:
    echo   - test_init.exe (initialization test)
    echo   - dropblocks_debug.exe (debug version with safety checks)
    echo   - dropblocks.exe (production version)
    echo.
    echo To test initialization first:
    echo   test_init.exe           # Test SDL2 initialization step by step
    echo.
    echo To run the game:
    echo   dropblocks_debug.exe    # Debug version with safety checks
    echo   dropblocks.exe          # Production version
    echo.
    echo If you get segmentation fault, run:
    echo   gdb dropblocks_debug.exe # Debug full game
    echo   (gdb) run
    echo   (gdb) bt
    echo.
) else (
    echo ERROR: Build files not found
    pause
    exit /b 1
)

pause
