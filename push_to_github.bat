@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

where git >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [错误] 未检测到 git 命令，请先安装 Git for Windows ^(https://git-scm.com/download/win^)。
    pause
    exit /b 1
)

echo 正在初始化本地 Git 仓库并关联远程地址 https://github.com/Ftozon/DesktopShredder.git ...
if not exist ".git" (
    git init
)
git branch -M main
git remote remove origin >nul 2>nul
git remote add origin https://github.com/Ftozon/DesktopShredder.git

git add -A
git commit -m "feat: initial release of DesktopShredder (Win32 native v1.2, WPF, and macOS AppKit)" 2>nul

echo 正在同步远程分支（若弹出 GitHub 登录窗口请授权）...
git pull origin main --allow-unrelated-histories --no-edit 2>nul
git push -u origin main

if %ERRORLEVEL% equ 0 (
    echo.
    echo [成功] 全部源码已上传至 https://github.com/Ftozon/DesktopShredder !
) else (
    echo.
    echo [提示] 若远端存在冲突或需要覆盖推送，可执行: git push -u origin main --force
)
pause
