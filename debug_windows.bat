@echo off
REM ===========================
REM   DROPBLOCKS - DEBUG SCRIPT PARA WINDOWS
REM   Script para debugging de segmentation fault
REM ===========================

echo === DROPBLOCKS DEBUG SCRIPT FOR WINDOWS ===

REM Verificar se o executável existe
if not exist "dropblocks_debug.exe" (
    echo ERROR: dropblocks_debug.exe not found. Run build_windows.bat first.
    pause
    exit /b 1
)

REM Verificar dependências
echo Checking dependencies...
where dumpbin >nul 2>&1
if %errorlevel% equ 0 (
    echo Dependencies:
    dumpbin /dependents dropblocks_debug.exe
) else (
    echo Dependencies check not available (dumpbin not found)
)

echo.
echo === RUNNING INITIALIZATION TEST ===
if exist "test_init.exe" (
    echo Running initialization test...
    test_init.exe
    echo.
) else (
    echo test_init.exe not found, skipping initialization test
)

echo === RUNNING WITH GDB (if available) ===
where gdb >nul 2>&1
if %errorlevel% equ 0 (
    echo Starting GDB debugger...
    echo Commands to use in GDB:
    echo   (gdb) run
    echo   (gdb) bt          # Show backtrace when it crashes
    echo   (gdb) info registers
    echo   (gdb) x/20x $rsp  # Examine stack
    echo   (gdb) quit
    echo.
    gdb dropblocks_debug.exe
) else (
    echo GDB not available. Installing MinGW-w64 or MSYS2 is recommended.
    echo Running without debugger...
    dropblocks_debug.exe
)

pause
