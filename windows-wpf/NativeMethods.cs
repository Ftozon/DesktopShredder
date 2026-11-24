using System;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;

namespace DesktopShredder
{
    internal static class NativeMethods
    {
        public const int GWL_STYLE = -16;
        public const int GWL_EXSTYLE = -20;

        public const uint WS_CAPTION = 0x00C00000;
        public const int WS_EX_TOOLWINDOW = 0x00000080;
        public const int WS_EX_NOACTIVATE = 0x08000000;

        public const uint MONITOR_DEFAULTTONEAREST = 0x00000002;

        public enum QUERY_USER_NOTIFICATION_STATE
        {
            QUNS_NOT_PRESENT = 1,
            QUNS_BUSY = 2,
            QUNS_RUNNING_D3D_FULL_SCREEN = 3,
            QUNS_PRESENTATION_MODE = 4,
            QUNS_ACCEPTS_NOTIFICATIONS = 5,
            QUNS_QUIET_TIME = 6,
            QUNS_APP = 7
        }

        public enum StreamInfoLevels
        {
            FindStreamInfoStandard = 0
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;

            public int Width => Right - Left;
            public int Height => Bottom - Top;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct MONITORINFO
        {
            public int cbSize;
            public RECT rcMonitor;
            public RECT rcWork;
            public uint dwFlags;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct WIN32_FIND_STREAM_DATA
        {
            public long StreamSize;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 296)]
            public string cStreamName;
        }

        [DllImport("user32.dll")]
        public static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsIconic(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport("user32.dll")]
        public static extern IntPtr MonitorFromWindow(IntPtr hwnd, uint dwFlags);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetMonitorInfoW(IntPtr hMonitor, ref MONITORINFO lpmi);

        [DllImport("user32.dll", EntryPoint = "GetWindowLongPtrW")]
        private static extern IntPtr GetWindowLongPtr64(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", EntryPoint = "GetWindowLongW")]
        private static extern int GetWindowLong32(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", EntryPoint = "SetWindowLongPtrW")]
        private static extern IntPtr SetWindowLongPtr64(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

        [DllImport("user32.dll", EntryPoint = "SetWindowLongW")]
        private static extern int SetWindowLong32(IntPtr hWnd, int nIndex, int dwNewLong);

        public static long GetWindowLong(IntPtr hWnd, int nIndex)
        {
            return IntPtr.Size == 8
                ? GetWindowLongPtr64(hWnd, nIndex).ToInt64()
                : GetWindowLong32(hWnd, nIndex);
        }

        public static void SetWindowLong(IntPtr hWnd, int nIndex, long dwNewLong)
        {
            if (IntPtr.Size == 8)
            {
                SetWindowLongPtr64(hWnd, nIndex, new IntPtr(dwNewLong));
            }
            else
            {
                SetWindowLong32(hWnd, nIndex, (int)dwNewLong);
            }
        }

        [DllImport("shell32.dll")]
        public static extern int SHQueryUserNotificationState(out QUERY_USER_NOTIFICATION_STATE pquns);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FlushFileBuffers(SafeFileHandle hFile);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr FindFirstStreamW(
            string lpFileName,
            StreamInfoLevels InfoLevel,
            out WIN32_FIND_STREAM_DATA lpFindStreamData,
            uint dwFlags);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FindNextStreamW(
            IntPtr hFindStream,
            out WIN32_FIND_STREAM_DATA lpFindStreamData);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FindClose(IntPtr hFindFile);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DeleteFileW(string lpFileName);

        public static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);

        /// <summary>
        /// Accurately detects whether another application is running in fullscreen mode
        /// (covering exclusive DirectX/Vulkan games, presentation mode, F11 browsers, and fullscreen video players)
        /// without falsely triggering on normal maximized desktop windows or the Windows desktop shell itself.
        /// </summary>
        public static bool IsAnotherAppFullscreen(uint currentProcessId)
        {
            if (SHQueryUserNotificationState(out QUERY_USER_NOTIFICATION_STATE state) == 0)
            {
                if (state == QUERY_USER_NOTIFICATION_STATE.QUNS_RUNNING_D3D_FULL_SCREEN ||
                    state == QUERY_USER_NOTIFICATION_STATE.QUNS_PRESENTATION_MODE)
                {
                    return true;
                }
            }

            IntPtr fgWindow = GetForegroundWindow();
            if (fgWindow == IntPtr.Zero || !IsWindowVisible(fgWindow) || IsIconic(fgWindow))
            {
                return false;
            }

            GetWindowThreadProcessId(fgWindow, out uint fgPid);
            if (fgPid == 0 || fgPid == currentProcessId)
            {
                return false;
            }

            StringBuilder classBuilder = new StringBuilder(128);
            int chars = GetClassNameW(fgWindow, classBuilder, classBuilder.Capacity);
            if (chars > 0)
            {
                string cls = classBuilder.ToString();
                // Ignore Windows desktop wallpaper host, taskbar, and shell overlays
                if (string.Equals(cls, "Progman", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(cls, "WorkerW", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(cls, "Shell_TrayWnd", StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(cls, "Shell_SecondaryTrayWnd", StringComparison.OrdinalIgnoreCase))
                {
                    return false;
                }
            }

            if (!GetWindowRect(fgWindow, out RECT winRect))
            {
                return false;
            }

            IntPtr hMonitor = MonitorFromWindow(fgWindow, MONITOR_DEFAULTTONEAREST);
            if (hMonitor == IntPtr.Zero)
            {
                return false;
            }

            MONITORINFO mi = new MONITORINFO { cbSize = Marshal.SizeOf<MONITORINFO>() };
            if (!GetMonitorInfoW(hMonitor, ref mi))
            {
                return false;
            }

            bool coversFullMonitor =
                winRect.Left <= mi.rcMonitor.Left &&
                winRect.Top <= mi.rcMonitor.Top &&
                winRect.Right >= mi.rcMonitor.Right &&
                winRect.Bottom >= mi.rcMonitor.Bottom;

            if (!coversFullMonitor)
            {
                return false;
            }

            // If taskbar is set to auto-hide, rcWork equals rcMonitor; ensure a normal titled maximized window
            // is not mistaken for fullscreen by checking whether WS_CAPTION has been stripped.
            bool taskbarAutoHidden =
                mi.rcWork.Left == mi.rcMonitor.Left &&
                mi.rcWork.Top == mi.rcMonitor.Top &&
                mi.rcWork.Right == mi.rcMonitor.Right &&
                mi.rcWork.Bottom == mi.rcMonitor.Bottom;

            if (taskbarAutoHidden)
            {
                long style = GetWindowLong(fgWindow, GWL_STYLE);
                if ((style & WS_CAPTION) == WS_CAPTION)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
