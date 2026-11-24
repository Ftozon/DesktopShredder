#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[ -d .git ] || git init
git branch -M main
git remote remove origin 2>/dev/null || true
git remote add origin https://github.com/Ftozon/DesktopShredder.git
git add -A
git commit -m 'feat: initial release of DesktopShredder (Win32 native v1.2, WPF, and macOS AppKit)' || true
git pull origin main --allow-unrelated-histories --no-edit 2>/dev/null || true
git push -u origin main
echo '[成功] 全部源码已推送至 https://github.com/Ftozon/DesktopShredder !'
