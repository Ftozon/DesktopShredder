import Darwin
import Foundation
import Security

/// High-assurance unrecoverable file & directory shredder for macOS (Darwin/POSIX).
/// Implements:
/// 1. Symlink boundary protection via `lstat` + `O_NOFOLLOW` (never traverses into link targets).
/// 2. Extended Attributes (`xattr`, FinderInfo, Quarantine, WhereFroms) cryptographic overwrite & removal.
/// 3. Kernel unified cache bypass (`fcntl(fd, F_NOCACHE, 1)`) + physical non-volatile drive flush (`fcntl(fd, F_FULLFSYNC, 0)`).
/// 4. Multi-pass overwrite (Default: 7-pass DoD 5220.22-M ECE + SecRandomCopyBytes CSPRNG; supports 3 / 7 / 35 passes).
/// 5. Zero-byte truncation (`ftruncate`), epoch timestamp scrub (`utimes`), and 3-round directory entry rename before `unlink`.
/// 6. Working buffer zeroization (`memset_s`) and strict zero-logging policy.
enum SecureShredder {
    private static let bufferSize = 1024 * 1024 // 1 MiB page-aligned DMA buffer
    private static let antiForensicEpochSec: __darwin_time_t = 315_532_800 // 1980-01-01 00:00:00 UTC
    private static let safeChars = Array("abcdefghijklmnopqrstuvwxyz0123456789".utf8)

    private static let gutmannPatterns: [[UInt8]] = [
        [0x55], [0xAA],
        [0x92, 0x49, 0x24], [0x49, 0x24, 0x92], [0x24, 0x92, 0x49],
        [0x00], [0x11], [0x22], [0x33], [0x44], [0x55], [0x66], [0x77],
        [0x88], [0x99], [0xAA], [0xBB], [0xCC], [0xDD], [0xEE], [0xFF],
        [0x92, 0x49, 0x24], [0x49, 0x24, 0x92], [0x24, 0x92, 0x49],
        [0x6D, 0xB6, 0xDB], [0xB6, 0xDB, 0x6D], [0xDB, 0x6D, 0xB6]
    ]

    @discardableResult
    static func shredTargets(_ paths: [String], passCount: Int) -> (succeeded: Int, failed: Int) {
        var ok = 0
        var fail = 0
        for rawPath in paths where !rawPath.isEmpty {
            if shredEntry(rawPath, passCount: passCount) {
                ok += 1
            } else {
                fail += 1
            }
        }
        return (ok, fail)
    }

    private static func shredEntry(_ path: String, passCount: Int) -> Bool {
        var st = stat()
        guard lstat(path, &st) == 0 else {
            // Already absent
            return true
        }

        let fileType = st.st_mode & S_IFMT

        // Never follow symbolic links: unlink only the symlink inode itself
        if fileType == S_IFLNK {
            _ = chflags(path, 0)
            return unlink(path) == 0
        }

        if fileType == S_IFDIR {
            return shredDirectoryRecursive(path, passCount: passCount)
        }

        return shredSingleFile(path, passCount: passCount)
    }

    private static func shredDirectoryRecursive(_ dirPath: String, passCount: Int) -> Bool {
        _ = chflags(dirPath, 0)
        _ = chmod(dirPath, mode_t(S_IRWXU))

        guard let dirPtr = opendir(dirPath) else {
            return false
        }

        var childPaths: [String] = []
        while let entry = readdir(dirPtr) {
            let name = withUnsafePointer(to: entry.pointee.d_name) { ptr -> String in
                ptr.withMemoryRebound(to: CChar.self, capacity: Int(entry.pointee.d_namlen) + 1) { cStr in
                    String(cString: cStr)
                }
            }
            if name == "." || name == ".." {
                continue
            }
            childPaths.append((dirPath as NSString).appendingPathComponent(name))
        }
        closedir(dirPtr)

        var allOk = true
        for child in childPaths {
            if !shredEntry(child, passCount: passCount) {
                allOk = false
            }
        }

        if !allOk {
            return false
        }

        let finalDir = scrambleEntryName(dirPath, isDirectory: true)
        resetTimestamps(finalDir)
        return rmdir(finalDir) == 0
    }

    private static func shredSingleFile(_ filePath: String, passCount: Int) -> Bool {
        // 1. Clear BSD immutable/append flags & grant owner RW
        _ = chflags(filePath, 0)
        _ = chmod(filePath, mode_t(S_IRUSR | S_IWUSR))

        // 2. Scrub and remove all macOS Extended Attributes (FinderInfo, Quarantine, WhereFroms, etc.)
        wipeExtendedAttributes(filePath)

        // 3. Open without following symlinks & disable unified kernel buffer caching
        let fd = open(filePath, O_RDWR | O_NOFOLLOW)
        if fd >= 0 {
            _ = fcntl(fd, F_NOCACHE, 1)

            var fst = stat()
            if fstat(fd, &fst) == 0 && fst.st_size > 0 {
                let length = Int64(fst.st_size)
                let rawBuf = UnsafeMutableRawPointer.allocate(byteCount: bufferSize, alignment: 4096)
                defer {
                    _ = memset_s(rawBuf, bufferSize, 0, bufferSize)
                    rawBuf.deallocate()
                }

                executeOverwritePasses(fd: fd, length: length, passCount: passCount, buffer: rawBuf)
            }

            // 4. Truncate file size to 0 and force hardware controller flush
            _ = ftruncate(fd, 0)
            flushPhysicalMedia(fd: fd)
            close(fd)
        }

        // 5. Reset timestamps to anti-forensic epoch
        resetTimestamps(filePath)

        // 6. Scramble directory entry name 3 times before unlinking
        let finalPath = scrambleEntryName(filePath, isDirectory: false)
        return unlink(finalPath) == 0
    }

    private static func executeOverwritePasses(
        fd: Int32,
        length: Int64,
        passCount: Int,
        buffer: UnsafeMutableRawPointer
    ) {
        if passCount >= 35 {
            for _ in 0..<4 {
                writePassRandom(fd: fd, length: length, buffer: buffer)
            }
            for pattern in gutmannPatterns {
                writePassPattern(fd: fd, length: length, buffer: buffer, pattern: pattern)
            }
            for _ in 0..<4 {
                writePassRandom(fd: fd, length: length, buffer: buffer)
            }
        } else if passCount <= 3 {
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0x00)
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0xFF)
            writePassRandom(fd: fd, length: length, buffer: buffer)
        } else {
            // Default 7-Pass DoD 5220.22-M ECE + CSPRNG
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0x00)
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0xFF)
            writePassRandom(fd: fd, length: length, buffer: buffer)
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0x55)
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0xAA)
            writePassRandom(fd: fd, length: length, buffer: buffer)
            writePassByte(fd: fd, length: length, buffer: buffer, byte: 0x00)
        }
    }

    private static func writePassByte(fd: Int32, length: Int64, buffer: UnsafeMutableRawPointer, byte: UInt8) {
        memset(buffer, Int32(byte), bufferSize)
        writeAcrossFile(fd: fd, length: length, buffer: buffer, refillRandom: false)
    }

    private static func writePassPattern(
        fd: Int32,
        length: Int64,
        buffer: UnsafeMutableRawPointer,
        pattern: [UInt8]
    ) {
        let typed = buffer.assumingMemoryBound(to: UInt8.self)
        if pattern.count == 1 {
            memset(buffer, Int32(pattern[0]), bufferSize)
        } else {
            for i in 0..<bufferSize {
                typed[i] = pattern[i % pattern.count]
            }
        }
        writeAcrossFile(fd: fd, length: length, buffer: buffer, refillRandom: false)
    }

    private static func writePassRandom(fd: Int32, length: Int64, buffer: UnsafeMutableRawPointer) {
        writeAcrossFile(fd: fd, length: length, buffer: buffer, refillRandom: true)
    }

    private static func writeAcrossFile(
        fd: Int32,
        length: Int64,
        buffer: UnsafeMutableRawPointer,
        refillRandom: Bool
    ) {
        _ = lseek(fd, 0, SEEK_SET)
        var remaining = length

        while remaining > 0 {
            let chunk = Int(min(Int64(bufferSize), remaining))
            if refillRandom {
                _ = SecRandomCopyBytes(kSecRandomDefault, chunk, buffer)
            }

            var offset = 0
            while offset < chunk {
                let written = Darwin.write(fd, buffer.advanced(by: offset), chunk - offset)
                if written <= 0 {
                    if errno == EINTR { continue }
                    return
                }
                offset += written
            }
            remaining -= Int64(chunk)
        }

        flushPhysicalMedia(fd: fd)
    }

    private static func flushPhysicalMedia(fd: Int32) {
        if fcntl(fd, F_FULLFSYNC, 0) == -1 {
            _ = fsync(fd)
        }
    }

    private static func wipeExtendedAttributes(_ path: String) {
        let listSize = listxattr(path, nil, 0, XATTR_NOFOLLOW)
        guard listSize > 0 else { return }

        var nameBuffer = [CChar](repeating: 0, count: listSize)
        let actualSize = listxattr(path, &nameBuffer, listSize, XATTR_NOFOLLOW)
        guard actualSize > 0 else { return }

        var attrNames: [String] = []
        var idx = 0
        while idx < actualSize {
            let name = nameBuffer.withUnsafeBufferPointer { buf -> String in
                guard let base = buf.baseAddress else { return "" }
                return String(cString: base.advanced(by: idx))
            }
            if !name.isEmpty {
                attrNames.append(name)
            }
            idx += name.utf8.count + 1
        }

        for attr in attrNames {
            let valLen = getxattr(path, attr, nil, 0, 0, XATTR_NOFOLLOW)
            if valLen > 0 && valLen <= 1024 * 1024 {
                var scrub = [UInt8](repeating: 0, count: valLen)
                _ = SecRandomCopyBytes(kSecRandomDefault, valLen, &scrub)
                _ = setxattr(path, attr, &scrub, valLen, 0, XATTR_NOFOLLOW)
            }
            _ = removexattr(path, attr, XATTR_NOFOLLOW)
        }
    }

    private static func resetTimestamps(_ path: String) {
        var times = [
            timeval(tv_sec: antiForensicEpochSec, tv_usec: 0),
            timeval(tv_sec: antiForensicEpochSec, tv_usec: 0)
        ]
        _ = utimes(path, &times)
    }

    private static func scrambleEntryName(_ originalPath: String, isDirectory: Bool) -> String {
        let nsPath = originalPath as NSString
        let parent = nsPath.deletingLastPathComponent
        guard !parent.isEmpty else { return originalPath }

        let origName = nsPath.lastPathComponent
        let firstLen = min(max(origName.count, 4), 64)
        let lengths = [firstLen, isDirectory ? 6 : 8, 1]

        var current = originalPath
        for len in lengths {
            for _ in 0..<6 {
                let candidate = (parent as NSString).appendingPathComponent(randomName(length: len))
                var tmpStat = stat()
                if lstat(candidate, &tmpStat) != 0 {
                    if Darwin.rename(current, candidate) == 0 {
                        current = candidate
                    }
                    break
                }
            }
        }
        return current
    }

    private static func randomName(length: Int) -> String {
        var bytes = [UInt8](repeating: 0, count: length)
        _ = SecRandomCopyBytes(kSecRandomDefault, length, &bytes)
        let mapped = bytes.map { safeChars[Int($0) % safeChars.count] }
        return String(decoding: mapped, as: UTF8.self)
    }
}
