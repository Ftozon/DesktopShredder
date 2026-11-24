#!/usr/bin/env python3
"""Compile desktop_shredder_win32.c v1.2 into DesktopShredder_v1.2.exe, pack zip, and verify PE imports."""

import shutil
import subprocess
import zipfile
from pathlib import Path
from build_pe64 import compile_pe64

BASE = Path("/working_dir/c_daa86fdf95104dc7/artifacts/file_generation/ttl=63d")
SRC = BASE / "intermediate" / "desktop_shredder_win32.c"
ICO = BASE / "intermediate" / "project_stage" / "DesktopShredder" / "windows" / "Assets" / "shredder.ico"
OUT_EXE = BASE / "output" / "DesktopShredder_v1.2.exe"
OUT_ZIP = BASE / "output" / "DesktopShredder_v1.2.zip"
WORK = BASE / "intermediate" / "pe_build_shredder_v12"

IMPORTS = {
    "KERNEL32.dll": [
        "ExitProcess",
        "GetModuleHandleW",
        "GetModuleFileNameW",
        "GetCurrentProcessId",
        "GetLastError",
        "CreateMutexW",
        "CreateEventW",
        "SetEvent",
        "WaitForSingleObject",
        "Sleep",
        "CreateThread",
        "AcquireSRWLockExclusive",
        "ReleaseSRWLockExclusive",
        "GetProcessHeap",
        "HeapAlloc",
        "HeapFree",
        "VirtualAlloc",
        "VirtualFree",
        "GetEnvironmentVariableW",
        "CreateDirectoryW",
        "GetFileAttributesW",
        "SetFileAttributesW",
        "CreateFileW",
        "GetFileSizeEx",
        "SetFilePointerEx",
        "SetEndOfFile",
        "WriteFile",
        "ReadFile",
        "FlushFileBuffers",
        "SetFileTime",
        "CloseHandle",
        "MoveFileExW",
        "DeleteFileW",
        "RemoveDirectoryW",
        "FindFirstFileW",
        "FindNextFileW",
        "FindClose",
        "FindFirstStreamW",
        "FindNextStreamW",
    ],
    "USER32.dll": [
        "RegisterClassExW",
        "FindWindowW",
        "CreateWindowExW",
        "DestroyWindow",
        "ShowWindow",
        "UpdateWindow",
        "IsWindowVisible",
        "IsIconic",
        "SetForegroundWindow",
        "GetMessageW",
        "TranslateMessage",
        "DispatchMessageW",
        "PostQuitMessage",
        "PostMessageW",
        "SendMessageW",
        "DefWindowProcW",
        "LoadCursorW",
        "LoadIconW",
        "GetDC",
        "ReleaseDC",
        "UpdateLayeredWindow",
        "GetWindowRect",
        "SetWindowPos",
        "GetCursorPos",
        "SetCapture",
        "ReleaseCapture",
        "SetTimer",
        "KillTimer",
        "CreatePopupMenu",
        "AppendMenuW",
        "TrackPopupMenu",
        "DestroyMenu",
        "GetSystemMetrics",
        "GetForegroundWindow",
        "GetWindowThreadProcessId",
        "GetClassNameW",
        "GetWindowLongPtrW",
        "MonitorFromWindow",
        "GetMonitorInfoW",
        "ChangeWindowMessageFilterEx",
    ],
    "GDI32.dll": [
        "CreateCompatibleDC",
        "DeleteDC",
        "CreateDIBSection",
        "SelectObject",
        "DeleteObject",
        "CreateFontW",
        "CreateSolidBrush",
        "GetDeviceCaps",
    ],
    "SHELL32.dll": [
        "DragAcceptFiles",
        "DragQueryFileW",
        "DragFinish",
        "SHQueryUserNotificationState",
    ],
    "ADVAPI32.dll": [
        "RegCreateKeyExW",
        "RegOpenKeyExW",
        "RegSetValueExW",
        "RegQueryValueExW",
        "RegDeleteValueW",
        "RegCloseKey",
    ],
    "BCRYPT.dll": [
        "BCryptGenRandom",
    ],
    "OLE32.dll": [
        "OleInitialize",
        "OleUninitialize",
        "RegisterDragDrop",
        "RevokeDragDrop",
        "ReleaseStgMedium",
    ],
}

if __name__ == "__main__":
    if WORK.exists():
        shutil.rmtree(WORK)
    compile_pe64([SRC], IMPORTS, ICO, OUT_EXE, WORK)
    with zipfile.ZipFile(OUT_ZIP, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        zf.write(OUT_EXE, arcname="DesktopShredder_v1.2.exe")

    relocs = subprocess.check_output(["readelf", "-rW", str(WORK / "final.elf")], text=True)
    assert "R_X86_64_64" not in relocs, "Found unexpected absolute 64-bit relocations!"
    pe_dump = subprocess.check_output(["objdump", "-p", str(OUT_EXE)], text=True)
    assert "OLE32.dll" in pe_dump and "RegisterDragDrop" in pe_dump
    print("[OK] Zero absolute relocations & OLE32.dll imports verified cleanly!")
