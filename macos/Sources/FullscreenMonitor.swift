import AppKit
import CoreGraphics
import Foundation

/// Detects whether another application currently occupies fullscreen on macOS
/// (covering both native macOS FullScreen Spaces and borderless fullscreen video/game windows)
/// without accessing `kCGWindowName` (so zero Screen Recording privacy prompts are triggered).
final class FullscreenMonitor {
    private let currentPid: pid_t = ProcessInfo.processInfo.processIdentifier
    private let onStateChange: (Bool) -> Void
    private var pollTimer: Timer?
    private var lastState: Bool = false

    private static let ignoredBundleIds: Set<String> = [
        "com.apple.finder",
        "com.apple.dock",
        "com.apple.controlcenter",
        "com.apple.notificationcenterui",
        "com.apple.WindowManager",
        "com.apple.WallpaperAgent",
        "com.apple.loginwindow"
    ]

    init(onStateChange: @escaping (Bool) -> Void) {
        self.onStateChange = onStateChange
    }

    func start() {
        let wsCenter = NSWorkspace.shared.notificationCenter
        wsCenter.addObserver(
            self,
            selector: #selector(handleWorkspaceEvent(_:)),
            name: NSWorkspace.activeSpaceDidChangeNotification,
            object: nil
        )
        wsCenter.addObserver(
            self,
            selector: #selector(handleWorkspaceEvent(_:)),
            name: NSWorkspace.didActivateApplicationNotification,
            object: nil
        )

        let timer = Timer(timeInterval: 0.45, repeats: true) { [weak self] _ in
            self?.evaluateFullscreenState()
        }
        RunLoop.main.add(timer, forMode: .common)
        pollTimer = timer
        evaluateFullscreenState()
    }

    func stop() {
        pollTimer?.invalidate()
        pollTimer = nil
        NSWorkspace.shared.notificationCenter.removeObserver(self)
    }

    @objc private func handleWorkspaceEvent(_ notification: Notification) {
        evaluateFullscreenState()
    }

    private func evaluateFullscreenState() {
        let isFullscreen = detectOtherAppFullscreen()
        if isFullscreen != lastState {
            lastState = isFullscreen
            onStateChange(isFullscreen)
        }
    }

    private func detectOtherAppFullscreen() -> Bool {
        guard let frontApp = NSWorkspace.shared.frontmostApplication else {
            return false
        }

        let frontPid = frontApp.processIdentifier
        if frontPid == currentPid {
            return false
        }

        if let bundleId = frontApp.bundleIdentifier, Self.ignoredBundleIds.contains(bundleId) {
            return false
        }

        let screens = NSScreen.screens
        guard !screens.isEmpty,
              let primaryScreenHeight = screens.first?.frame.height,
              let windowList = CGWindowListCopyWindowInfo(
                  [.optionOnScreenOnly, .excludeDesktopElements],
                  kCGNullWindowID
              ) as? [[String: Any]] else {
            return false
        }

        // Convert NSScreen frames (bottom-left origin) to CoreGraphics global display coordinates (top-left origin)
        let cgScreenFrames: [CGRect] = screens.map { screen in
            let f = screen.frame
            return CGRect(
                x: f.origin.x,
                y: primaryScreenHeight - f.maxY,
                width: f.width,
                height: f.height
            )
        }

        for info in windowList {
            guard let ownerPid = info[kCGWindowOwnerPID as String] as? pid_t,
                  ownerPid == frontPid else {
                continue
            }

            if let alpha = info[kCGWindowAlpha as String] as? Double, alpha <= 0.01 {
                continue
            }

            let layer = (info[kCGWindowLayer as String] as? Int) ?? 0
            if layer < 0 {
                continue
            }

            guard let boundsDict = info[kCGWindowBounds as String] as? NSDictionary,
                  let winRect = CGRect(dictionaryRepresentation: boundsDict as CFDictionary) else {
                continue
            }

            // Normal zoomed windows on macOS never cover the top Menu Bar (24pt–38pt),
            // whereas both FullScreen Space windows and borderless fullscreen apps cover the full screen rect.
            for screenRect in cgScreenFrames {
                let coversScreen =
                    abs(winRect.minX - screenRect.minX) <= 2.0 &&
                    abs(winRect.minY - screenRect.minY) <= 2.0 &&
                    winRect.width >= screenRect.width - 2.0 &&
                    winRect.height >= screenRect.height - 2.0
                if coversScreen {
                    return true
                }
            }
        }

        return false
    }
}
