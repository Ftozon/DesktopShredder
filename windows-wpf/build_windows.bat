@echo off
chcp 65001 >nul
setlocal

echo ========================================================
echo   正在编译 Windows 原生单文件版 [桌面碎纸机 DesktopShredder]
echo ========================================================

where dotnet >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [错误] 未检测到 .NET 8 SDK。请先安装 .NET 8 SDK ^(https://dotnet.microsoft.com/download/dotnet/8.0^) 后重试。
    pause
    exit /b 1
)

dotnet publish DesktopShredder.csproj ^
    -c Release ^
    -r win-x64 ^
    --self-contained true ^
    -p:PublishSingleFile=true ^
    -p:IncludeNativeLibrariesForSelfExtract=true ^
    -p:DebugType=None ^
    -p:DebugSymbols=false ^
    -o dist

if %ERRORLEVEL% neq 0 (
    echo [失败] 编译过程中出现错误。
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [完成] 已生成免安装独立单文件程序：
echo   %~dp0dist\DesktopShredder.exe
echo 直接双击运行即可悬浮于桌面，无需联网或安装额外运行时！
pause
