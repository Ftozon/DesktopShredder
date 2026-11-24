using System;
using System.Diagnostics;
using Microsoft.Win32;

namespace DesktopShredder
{
    /// <summary>
    /// Manages Windows current-user launch-at-startup via HKCU\Software\Microsoft\Windows\CurrentVersion\Run.
    /// Does not require Administrator elevation.
    /// </summary>
    internal static class StartupManager
    {
        private const string RunRegistryPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string AppRegistryName = "DesktopShredder";

        public static bool IsAutoStartEnabled()
        {
            try
            {
                using RegistryKey? key = Registry.CurrentUser.OpenSubKey(RunRegistryPath, writable: false);
                object? val = key?.GetValue(AppRegistryName);
                return val is string s && !string.IsNullOrWhiteSpace(s);
            }
            catch
            {
                return false;
            }
        }

        public static bool SetAutoStart(bool enable)
        {
            try
            {
                using RegistryKey? key = Registry.CurrentUser.OpenSubKey(RunRegistryPath, writable: true)
                    ?? Registry.CurrentUser.CreateSubKey(RunRegistryPath, writable: true);

                if (key == null)
                {
                    return false;
                }

                if (enable)
                {
                    string? exePath = Environment.ProcessPath ?? Process.GetCurrentProcess().MainModule?.FileName;
                    if (string.IsNullOrWhiteSpace(exePath))
                    {
                        return false;
                    }
                    key.SetValue(AppRegistryName, $"\"{exePath}\"", RegistryValueKind.String);
                }
                else
                {
                    key.DeleteValue(AppRegistryName, throwOnMissingValue: false);
                }

                return true;
            }
            catch
            {
                return false;
            }
        }
    }
}
