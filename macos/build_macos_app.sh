#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "========================================================"
echo "  正在构建 macOS 原生版 [桌面碎纸机 DesktopShredder.app]"
echo "========================================================"

if ! command -v swiftc >/dev/null 2>&1; then
    echo "[错误] 未检测到 swiftc 编译器。请先在终端运行: xcode-select --install"
    exit 1
fi

APP_NAME="DesktopShredder"
APP_BUNDLE="dist/${APP_NAME}.app"
CONTENTS_DIR="${APP_BUNDLE}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

rm -rf "${APP_BUNDLE}"
mkdir -p "${MACOS_DIR}" "${RESOURCES_DIR}"

# 如系统自带 iconutil，可从 AppIcon.iconset 重新打包原生 icns；否则直接使用预生成的 AppIcon.icns
if command -v iconutil >/dev/null 2>&1 && [ -d "Resources/AppIcon.iconset" ]; then
    iconutil -c icns "Resources/AppIcon.iconset" -o "${RESOURCES_DIR}/AppIcon.icns" || cp "Resources/AppIcon.icns" "${RESOURCES_DIR}/AppIcon.icns"
else
    cp "Resources/AppIcon.icns" "${RESOURCES_DIR}/AppIcon.icns"
fi

cp "Resources/Info.plist" "${CONTENTS_DIR}/Info.plist"

SWIFT_SOURCES=(
    "Sources/AppConfig.swift"
    "Sources/StartupManager.swift"
    "Sources/SecureShredder.swift"
    "Sources/FullscreenMonitor.swift"
    "Sources/ShredderWidgetView.swift"
    "Sources/SettingsWindowController.swift"
    "Sources/main.swift"
)

echo "[1/2] 正在编译原生二进制程序..."
# 优先尝试构建 Apple Silicon + Intel 通用二进制 (Universal Binary)，若单架构 SDK 则自动回退本机架构
if swiftc -O "${SWIFT_SOURCES[@]}" -target arm64-apple-macos12.0 -o "${MACOS_DIR}/${APP_NAME}_arm64" 2>/dev/null && \
   swiftc -O "${SWIFT_SOURCES[@]}" -target x86_64-apple-macos12.0 -o "${MACOS_DIR}/${APP_NAME}_x86_64" 2>/dev/null && \
   command -v lipo >/dev/null 2>&1; then
    lipo -create -output "${MACOS_DIR}/${APP_NAME}" "${MACOS_DIR}/${APP_NAME}_arm64" "${MACOS_DIR}/${APP_NAME}_x86_64"
    rm -f "${MACOS_DIR}/${APP_NAME}_arm64" "${MACOS_DIR}/${APP_NAME}_x86_64"
    echo "      已生成 Universal (Apple Silicon + Intel) 通用二进制。"
else
    rm -f "${MACOS_DIR}/${APP_NAME}_arm64" "${MACOS_DIR}/${APP_NAME}_x86_64"
    swiftc -O "${SWIFT_SOURCES[@]}" -o "${MACOS_DIR}/${APP_NAME}"
    echo "      已生成当前 Mac 原生架构二进制。"
fi

chmod +x "${MACOS_DIR}/${APP_NAME}"

echo "[2/2] 正在写入禁网安全权限签名 (Ad-hoc Codesign + Offline Entitlements)..."
if command -v codesign >/dev/null 2>&1; then
    codesign --force --deep --sign - --entitlements "Resources/DesktopShredder.entitlements" "${APP_BUNDLE}"
fi

echo ""
echo "[完成] 已生成独立桌面应用："
echo "  $(pwd)/${APP_BUNDLE}"
echo "可直接拖入 /Applications（应用程序）双击启动，右键悬浮窗即可打开设置！"
