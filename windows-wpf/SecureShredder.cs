using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;

namespace DesktopShredder
{
    /// <summary>
    /// High-assurance unrecoverable file &amp; directory shredder for Windows.
    /// Implements:
    /// 1. Symlink / Junction reparse-point boundary guard (never traverses into link targets).
    /// 2. NTFS Alternate Data Streams (ADS) discovery, cryptographic overwrite, and deletion.
    /// 3. Direct hardware write-through (FILE_FLAG_WRITE_THROUGH + FlushFileBuffers) multi-pass overwrite
    ///    (Default: 7-pass DoD 5220.22-M ECE + CSPRNG; configurable 3 / 7 / 35 passes).
    /// 4. In-place resident $MFT data scrubbing before zero-length truncation.
    /// 5. Creation / Access / Modification UTC timestamp reset (anti-forensic timeline scrub).
    /// 6. Multi-round in-place MFT filename scrambling before Win32 DeleteFileW unlink.
    /// 7. Cryptographic zeroing of working memory buffers &amp; zero filename logging.
    /// </summary>
    internal static class SecureShredder
    {
        private const int BufferSize = 1024 * 1024; // 1 MiB write-through chunk
        private static readonly DateTime AntiForensicEpochUtc = new DateTime(1980, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        private static readonly char[] SafeNameChars = "abcdefghijklmnopqrstuvwxyz0123456789".ToCharArray();

        // Gutmann deterministic inner patterns (used when 35-pass mode is selected)
        private static readonly byte[][] GutmannPatterns = new byte[][]
        {
            new byte[] { 0x55 }, new byte[] { 0xAA },
            new byte[] { 0x92, 0x49, 0x24 }, new byte[] { 0x49, 0x24, 0x92 }, new byte[] { 0x24, 0x92, 0x49 },
            new byte[] { 0x00 }, new byte[] { 0x11 }, new byte[] { 0x22 }, new byte[] { 0x33 },
            new byte[] { 0x44 }, new byte[] { 0x55 }, new byte[] { 0x66 }, new byte[] { 0x77 },
            new byte[] { 0x88 }, new byte[] { 0x99 }, new byte[] { 0xAA }, new byte[] { 0xBB },
            new byte[] { 0xCC }, new byte[] { 0xDD }, new byte[] { 0xEE }, new byte[] { 0xFF },
            new byte[] { 0x92, 0x49, 0x24 }, new byte[] { 0x49, 0x24, 0x92 }, new byte[] { 0x24, 0x92, 0x49 },
            new byte[] { 0x6D, 0xB6, 0xDB }, new byte[] { 0xB6, 0xDB, 0x6D }, new byte[] { 0xDB, 0x6D, 0xB6 }
        };

        public static (int Succeeded, int Failed) ShredTargets(IEnumerable<string> paths, int passCount)
        {
            int ok = 0;
            int fail = 0;

            foreach (string rawPath in paths)
            {
                if (string.IsNullOrWhiteSpace(rawPath))
                {
                    continue;
                }

                try
                {
                    if (ShredEntry(rawPath, passCount))
                    {
                        ok++;
                    }
                    else
                    {
                        fail++;
                    }
                }
                catch
                {
                    fail++;
                }
            }

            return (ok, fail);
        }

        private static bool ShredEntry(string path, int passCount)
        {
            if (!File.Exists(path) && !Directory.Exists(path))
            {
                return true;
            }

            FileAttributes attrs = File.GetAttributes(path);
            bool isReparsePoint = (attrs & FileAttributes.ReparsePoint) != 0;
            bool isDirectory = (attrs & FileAttributes.Directory) != 0;

            // Never follow junctions or symbolic links into target folders/files
            if (isReparsePoint)
            {
                File.SetAttributes(path, FileAttributes.Normal);
                if (isDirectory)
                {
                    Directory.Delete(path, false);
                }
                else
                {
                    File.Delete(path);
                }
                return true;
            }

            if (isDirectory)
            {
                return ShredDirectoryRecursive(path, passCount);
            }

            return ShredSingleFile(path, passCount);
        }

        private static bool ShredDirectoryRecursive(string dirPath, int passCount)
        {
            bool allOk = true;

            try
            {
                File.SetAttributes(dirPath, FileAttributes.Normal);
            }
            catch
            {
                // Continue best-effort on special directory flags
            }

            string[] files = Array.Empty<string>();
            try
            {
                files = Directory.GetFiles(dirPath);
            }
            catch
            {
                allOk = false;
            }

            foreach (string file in files)
            {
                if (!ShredEntry(file, passCount))
                {
                    allOk = false;
                }
            }

            string[] subDirs = Array.Empty<string>();
            try
            {
                subDirs = Directory.GetDirectories(dirPath);
            }
            catch
            {
                allOk = false;
            }

            foreach (string sub in subDirs)
            {
                if (!ShredEntry(sub, passCount))
                {
                    allOk = false;
                }
            }

            if (!allOk)
            {
                return false;
            }

            // Scramble directory name in MFT before deleting empty directory
            string currentDir = ScrambleDirectoryName(dirPath);
            try
            {
                Directory.SetCreationTimeUtc(currentDir, AntiForensicEpochUtc);
                Directory.SetLastAccessTimeUtc(currentDir, AntiForensicEpochUtc);
                Directory.SetLastWriteTimeUtc(currentDir, AntiForensicEpochUtc);
            }
            catch
            {
                // Ignore FS timestamp granularity limits
            }

            Directory.Delete(currentDir, false);
            return true;
        }

        private static bool ShredSingleFile(string filePath, int passCount)
        {
            // 1. Strip ReadOnly / Hidden / System / Archive flags
            File.SetAttributes(filePath, FileAttributes.Normal);

            // 2. Wipe NTFS Alternate Data Streams (ADS) such as :Zone.Identifier or hidden streams
            WipeAlternateDataStreams(filePath);

            // 3. Multi-pass write-through physical overwrite of primary data stream
            byte[] buffer = new byte[BufferSize];
            try
            {
                using (FileStream fs = new FileStream(
                    filePath,
                    FileMode.Open,
                    FileAccess.ReadWrite,
                    FileShare.None,
                    BufferSize,
                    FileOptions.WriteThrough | FileOptions.SequentialScan))
                {
                    long length = fs.Length;
                    // For 0-byte or tiny files, still write a minimal 64-byte resident scrub block if length == 0?
                    // Only overwrite existing allocated/resident length so NTFS does not migrate resident MFT data
                    // leaving un-wiped bytes in the old MFT record!
                    if (length > 0)
                    {
                        ExecuteOverwritePasses(fs, length, passCount, buffer);
                    }

                    // 4. Truncate file length to 0 and flush metadata to storage media
                    fs.SetLength(0);
                    fs.Flush(true);
                    NativeMethods.FlushFileBuffers(fs.SafeFileHandle);
                }
            }
            finally
            {
                CryptographicOperations.ZeroMemory(buffer);
            }

            // 5. Reset file timestamps to erase forensic timeline metadata
            try
            {
                File.SetCreationTimeUtc(filePath, AntiForensicEpochUtc);
                File.SetLastAccessTimeUtc(filePath, AntiForensicEpochUtc);
                File.SetLastWriteTimeUtc(filePath, AntiForensicEpochUtc);
            }
            catch
            {
                // Ignore if underlying FS restricts epoch timestamps
            }

            // 6. Multi-round MFT filename scrambling (exact length first, then 8 chars, then 1 char)
            string finalPath = ScrambleFileName(filePath);

            // 7. Unlink via Win32 DeleteFileW (strictly bypasses Windows Recycle Bin)
            if (!NativeMethods.DeleteFileW(finalPath))
            {
                File.Delete(finalPath);
            }

            return !File.Exists(finalPath);
        }

        private static void ExecuteOverwritePasses(FileStream fs, long length, int passCount, byte[] buffer)
        {
            if (passCount >= 35)
            {
                // 35-Pass Gutmann Method: 4 CSPRNG + 27 deterministic patterns + 4 CSPRNG
                for (int i = 0; i < 4; i++)
                {
                    WritePassRandom(fs, length, buffer);
                }
                foreach (byte[] pattern in GutmannPatterns)
                {
                    WritePassPattern(fs, length, buffer, pattern);
                }
                for (int i = 0; i < 4; i++)
                {
                    WritePassRandom(fs, length, buffer);
                }
            }
            else if (passCount <= 3)
            {
                // 3-Pass DoD 5220.22-M: 0x00 -> 0xFF -> CSPRNG
                WritePassByte(fs, length, buffer, 0x00);
                WritePassByte(fs, length, buffer, 0xFF);
                WritePassRandom(fs, length, buffer);
            }
            else
            {
                // Default 7-Pass DoD 5220.22-M ECE + NIST SP 800-88 Purge:
                // Pass 1: 0x00 | Pass 2: 0xFF | Pass 3: CSPRNG | Pass 4: 0x55 | Pass 5: 0xAA | Pass 6: CSPRNG | Pass 7: 0x00
                WritePassByte(fs, length, buffer, 0x00);
                WritePassByte(fs, length, buffer, 0xFF);
                WritePassRandom(fs, length, buffer);
                WritePassByte(fs, length, buffer, 0x55);
                WritePassByte(fs, length, buffer, 0xAA);
                WritePassRandom(fs, length, buffer);
                WritePassByte(fs, length, buffer, 0x00);
            }
        }

        private static void WritePassByte(FileStream fs, long length, byte[] buffer, byte value)
        {
            Array.Fill(buffer, value);
            WriteBufferAcrossStream(fs, length, buffer, refillRandomEachChunk: false);
        }

        private static void WritePassPattern(FileStream fs, long length, byte[] buffer, byte[] pattern)
        {
            if (pattern.Length == 1)
            {
                Array.Fill(buffer, pattern[0]);
            }
            else
            {
                for (int i = 0; i < buffer.Length; i++)
                {
                    buffer[i] = pattern[i % pattern.Length];
                }
            }
            WriteBufferAcrossStream(fs, length, buffer, refillRandomEachChunk: false);
        }

        private static void WritePassRandom(FileStream fs, long length, byte[] buffer)
        {
            WriteBufferAcrossStream(fs, length, buffer, refillRandomEachChunk: true);
        }

        private static void WriteBufferAcrossStream(FileStream fs, long length, byte[] buffer, bool refillRandomEachChunk)
        {
            fs.Position = 0;
            long remaining = length;
            while (remaining > 0)
            {
                int toWrite = (int)Math.Min(buffer.Length, remaining);
                if (refillRandomEachChunk)
                {
                    RandomNumberGenerator.Fill(buffer.AsSpan(0, toWrite));
                }
                fs.Write(buffer, 0, toWrite);
                remaining -= toWrite;
            }
            fs.Flush(true);
            NativeMethods.FlushFileBuffers(fs.SafeFileHandle);
        }

        private static void WipeAlternateDataStreams(string filePath)
        {
            List<string> adsNames = new List<string>();
            IntPtr hFind = NativeMethods.FindFirstStreamW(
                filePath,
                NativeMethods.StreamInfoLevels.FindStreamInfoStandard,
                out NativeMethods.WIN32_FIND_STREAM_DATA streamData,
                0);

            if (hFind == NativeMethods.INVALID_HANDLE_VALUE || hFind == IntPtr.Zero)
            {
                return;
            }

            try
            {
                do
                {
                    string name = streamData.cStreamName;
                    // Skip the default unnamed data stream "::$DATA"
                    if (!string.IsNullOrEmpty(name) &&
                        !string.Equals(name, "::$DATA", StringComparison.OrdinalIgnoreCase) &&
                        name.EndsWith(":$DATA", StringComparison.OrdinalIgnoreCase))
                    {
                        adsNames.Add(name);
                    }
                }
                while (NativeMethods.FindNextStreamW(hFind, out streamData));
            }
            finally
            {
                NativeMethods.FindClose(hFind);
            }

            byte[] adsBuf = new byte[64 * 1024];
            try
            {
                foreach (string streamSuffix in adsNames)
                {
                    string fullAdsPath = filePath + streamSuffix;
                    try
                    {
                        using (FileStream afs = new FileStream(
                            fullAdsPath,
                            FileMode.Open,
                            FileAccess.ReadWrite,
                            FileShare.None,
                            adsBuf.Length,
                            FileOptions.WriteThrough))
                        {
                            long len = afs.Length;
                            if (len > 0)
                            {
                                WritePassByte(afs, len, adsBuf, 0x00);
                                WritePassRandom(afs, len, adsBuf);
                            }
                            afs.SetLength(0);
                            afs.Flush(true);
                            NativeMethods.FlushFileBuffers(afs.SafeFileHandle);
                        }
                        NativeMethods.DeleteFileW(fullAdsPath);
                    }
                    catch
                    {
                        // Continue wiping main stream even if an OS-locked ADS resists removal
                    }
                }
            }
            finally
            {
                CryptographicOperations.ZeroMemory(adsBuf);
            }
        }

        private static string ScrambleFileName(string originalPath)
        {
            string? parent = Path.GetDirectoryName(originalPath);
            if (string.IsNullOrEmpty(parent))
            {
                return originalPath;
            }

            string origName = Path.GetFileName(originalPath);
            int firstLen = Math.Clamp(origName.Length, 4, 64);
            int[] lengths = new int[] { firstLen, 8, 1 };

            string current = originalPath;
            foreach (int len in lengths)
            {
                for (int attempt = 0; attempt < 6; attempt++)
                {
                    string candidate = Path.Combine(parent, GenerateRandomName(len));
                    if (!File.Exists(candidate) && !Directory.Exists(candidate))
                    {
                        try
                        {
                            File.Move(current, candidate);
                            current = candidate;
                        }
                        catch
                        {
                            // Keep current name if filesystem rejects rename
                        }
                        break;
                    }
                }
            }

            return current;
        }

        private static string ScrambleDirectoryName(string originalDir)
        {
            string? parent = Path.GetDirectoryName(originalDir);
            if (string.IsNullOrEmpty(parent))
            {
                return originalDir;
            }

            string origName = Path.GetFileName(originalDir);
            int firstLen = Math.Clamp(origName.Length, 4, 64);
            int[] lengths = new int[] { firstLen, 6, 1 };

            string current = originalDir;
            foreach (int len in lengths)
            {
                for (int attempt = 0; attempt < 6; attempt++)
                {
                    string candidate = Path.Combine(parent, GenerateRandomName(len));
                    if (!File.Exists(candidate) && !Directory.Exists(candidate))
                    {
                        try
                        {
                            Directory.Move(current, candidate);
                            current = candidate;
                        }
                        catch
                        {
                        }
                        break;
                    }
                }
            }

            return current;
        }

        private static string GenerateRandomName(int length)
        {
            Span<char> chars = stackalloc char[length];
            Span<byte> rand = stackalloc byte[length];
            RandomNumberGenerator.Fill(rand);
            for (int i = 0; i < length; i++)
            {
                chars[i] = SafeNameChars[rand[i] % SafeNameChars.Length];
            }
            return new string(chars);
        }
    }
}
