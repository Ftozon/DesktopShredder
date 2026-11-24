# DesktopShredder（桌面碎纸机）

一款常驻桌面的纯本地、免登录、零网络权限**防恢复文件粉碎工具**，同时提供 **Windows 零依赖纯原生单文件版（Win32 C，仅 ~62 KB）**、**Windows WPF 版（C# .NET 8）** 与 **macOS 原生版（Swift AppKit + Darwin POSIX）**。

## 核心功能

- **三态桌面碎纸机交互图标**
  - **待机态**：深色金属质感桌面碎纸机悬浮窗，绿灯常亮待命，不占任务栏、`Alt+Tab` 或 macOS Dock。
  - **文件拖入悬停态**：鼠标拖着文件移到碎纸机上方时，图标即时切换为**单张折角文件图标**，移开后自动恢复。
  - **实时进纸粉碎动画**：松手释放文件后，播放纸张吸入刀口、刀头微震、刀齿转动与碎纸条飞落堆积动画，同时后台静默完成物理覆写销毁（无确认弹窗打扰）。
- **军工级防恢复安全粉碎（默认 7 次覆写）**
  - **防符号链接穿透**：自动识别符号链接（Symlink）与 NTFS 目录联接点（Junction），仅删除链接本身，绝不穿透误删目标目录。
  - **隐藏数据流与扩展属性擦除**：Windows 端自动覆写并删除 NTFS 备用数据流（ADS）；macOS 端覆写并清除所有 `xattr` 扩展属性（含下载来源等元数据）。
  - **绕过系统缓存直刷硬件**：Windows 端采用 `FILE_FLAG_WRITE_THROUGH` + `FlushFileBuffers`；macOS 端采用 `fcntl(fd, F_NOCACHE, 1)` + `fcntl(fd, F_FULLFSYNC)`。
  - **多轮覆写与痕迹抹除**：默认执行 7 轮覆写（`0x00` -> `0xFF` -> 系统密码学随机数 CSPRNG -> `0x55` -> `0xAA` -> CSPRNG -> `0x00`，可在设置中切换 3 / 7 / 35 轮 Gutmann 标准），随后截断文件为 0 字节、重置创建/访问/修改时间戳，并在同目录连续 3 轮随机重命名混淆目录项后再解链删除。
- **全屏自动避让与开机自启**
  - 实时检测全屏应用（DirectX/Vulkan 全屏游戏、PPT 放映、浏览器视频全屏及 macOS 原生全屏空间），全屏时自动隐藏，退出全屏自动恢复（普通最大化窗口不会误判）。
  - 右键点击悬浮窗即可打开**设置面板**，支持一键开关**开机自动启动**、锁定悬浮位置、切换覆写次数与调节不透明度。
- **100% 离线与零日志保证**
  - 可执行文件未导入任何网络通信动态库（无 `ws2_32` / `winhttp` / `wininet` / `URLSession`），macOS 端通过 Entitlements 显式禁用客户端与服务端网络权限；本地绝不记录任何被销毁的文件名。

## 仓库目录结构

```text
DesktopShredder/
├── releases/
│   └── DesktopShredder_v1.2.exe       # Windows 64位免安装可直接双击运行的独立单文件版
├── windows-native/                    # Windows 零依赖纯 Win32 C + OLE 拖放 + SDF 矢量渲染源码 (v1.2)
│   ├── desktop_shredder_win32.c       # 完整单文件原生 Win32 源码
│   ├── build_pe64.py                  # 确定性 PE32+ 交叉链接与图标/清单编译器
│   ├── compile_shredder_exe.py        # 构建与导出表自动化校验脚本
│   └── Assets/shredder.ico
├── windows-wpf/                       # Windows C# (.NET 8 WPF + Win32 P/Invoke) 工程版
├── macos/                             # macOS Swift (AppKit + Darwin POSIX) 原生工程版
│   ├── Sources/                       # Swift 源码
│   ├── Resources/                     # AppIcon.icns、Info.plist 与禁网 Entitlements
│   └── build_macos_app.sh             # 一键生成 DesktopShredder.app 脚本
└── assets/
    └── shredder_256.png               # 碎纸机高清预览图标
```

## 快速开始

### Windows 直接运行
直接下载并双击运行 [`releases/DesktopShredder_v1.2.exe`](releases/DesktopShredder_v1.2.exe)（适用于 64 位 Windows 10 / Windows 11，零运行库依赖）。

### macOS 编译运行
```bash
cd macos
chmod +x build_macos_app.sh
./build_macos_app.sh
open dist/DesktopShredder.app
```

## SSD / APFS 硬件安全建议
现代 NVMe/SSD 固态硬盘存在主控磨损均衡（Wear-Leveling）与 APFS 写时复制（Copy-on-Write）机制，建议同时开启系统自带的 **BitLocker**（Windows）或 **FileVault**（macOS）全盘加密，以防止闪存物理重映射页残留明文片段。
