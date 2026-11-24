@echo off
chcp 65001 >nul
echo Building DesktopShredder native Win64 executable...
gcc -O2 -std=c11 -municode -mwindows desktop_shredder_win32.c -lole32 -lbcrypt -lshell32 -ladvapi32 -lgdi32 -luser32 -o DesktopShredder.exe
