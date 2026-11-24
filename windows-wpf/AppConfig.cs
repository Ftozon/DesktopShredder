using System;
using System.IO;
using System.Text.Json;

namespace DesktopShredder
{
    /// <summary>
    /// Strictly offline local configuration (never records shredded file names or paths).
    /// Stored at %LOCALAPPDATA%\DesktopShredder\settings.json.
    /// </summary>
    public sealed class AppConfig
    {
        public double? WindowLeft { get; set; }
        public double? WindowTop { get; set; }
        public int ShredPasses { get; set; } = 7; // Default: 7-pass DoD 5220.22-M ECE + CSPRNG
        public double WidgetOpacity { get; set; } = 0.95;
        public bool LockPosition { get; set; } = false;

        private static string ConfigPath =>
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "DesktopShredder",
                "settings.json");

        public static AppConfig Load()
        {
            try
            {
                string path = ConfigPath;
                if (File.Exists(path))
                {
                    string json = File.ReadAllText(path);
                    AppConfig? loaded = JsonSerializer.Deserialize<AppConfig>(json);
                    if (loaded != null)
                    {
                        loaded.ShredPasses = NormalizePasses(loaded.ShredPasses);
                        loaded.WidgetOpacity = Math.Clamp(loaded.WidgetOpacity, 0.40, 1.0);
                        return loaded;
                    }
                }
            }
            catch
            {
                // Fall back to safe defaults if config file is absent or malformed
            }

            return new AppConfig();
        }

        public void Save()
        {
            try
            {
                string path = ConfigPath;
                string? dir = Path.GetDirectoryName(path);
                if (!string.IsNullOrEmpty(dir))
                {
                    Directory.CreateDirectory(dir);
                }

                ShredPasses = NormalizePasses(ShredPasses);
                WidgetOpacity = Math.Clamp(WidgetOpacity, 0.40, 1.0);

                JsonSerializerOptions opts = new JsonSerializerOptions { WriteIndented = true };
                File.WriteAllText(path, JsonSerializer.Serialize(this, opts));
            }
            catch
            {
                // Silently ignore write failures on read-only profiles
            }
        }

        private static int NormalizePasses(int passes)
        {
            if (passes >= 35) return 35;
            if (passes <= 3) return 3;
            return 7;
        }
    }
}
