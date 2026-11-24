import AppKit
import Foundation

final class AppDelegate: NSObject, NSApplicationDelegate {
    private let config = AppConfig.load()
    private let shredQueue = DispatchQueue(label: "com.local.desktopshredder.worker", qos: .userInitiated)
    private let widgetSize = NSSize(width: 96, height: 112)

    private var panel: ShredderPanel!
    private var widgetView: ShredderWidgetView!
    private var fullscreenMonitor: FullscreenMonitor?
    private var settingsController: SettingsWindowController?

    private var pendingJobs: Int = 0
    private let jobsLock = NSLock()
    private var hiddenByFullscreen: Bool = false

    func applicationDidFinishLaunching(_ notification: Notification) {
        let initialOrigin = computeInitialOrigin()
        let rect = NSRect(origin: initialOrigin, size: widgetSize)

        panel = ShredderPanel(contentRect: rect)
        widgetView = ShredderWidgetView(frame: NSRect(origin: .zero, size: widgetSize))

        widgetView.onFilesDropped = { [weak self] paths in
            self?.enqueueSilentShred(paths)
        }
        widgetView.onPositionChanged = { [weak self] newOrigin in
            guard let self = self else { return }
            self.config.windowX = Double(newOrigin.x)
            self.config.windowY = Double(newOrigin.y)
            self.config.save()
        }
        widgetView.onOpenSettings = { [weak self] in
            self?.showSettingsWindow()
        }
        widgetView.onResetPosition = { [weak self] in
            self?.resetPositionToBottomRight()
        }
        widgetView.onQuit = { [weak self] in
            self?.config.save()
            NSApplication.shared.terminate(nil)
        }

        panel.contentView = widgetView
        applyConfig()
        panel.orderFrontRegardless()

        fullscreenMonitor = FullscreenMonitor { [weak self] otherFullscreen in
            self?.handleFullscreenStateChanged(otherFullscreen)
        }
        fullscreenMonitor?.start()
    }

    func applicationWillTerminate(_ notification: Notification) {
        fullscreenMonitor?.stop()
    }

    private func computeInitialOrigin() -> NSPoint {
        let screens = NSScreen.screens
        let defaultScreen = NSScreen.main ?? screens.first
        let visible = defaultScreen?.visibleFrame ?? NSRect(x: 0, y: 0, width: 1440, height: 900)

        if let savedX = config.windowX, let savedY = config.windowY {
            let candidate = NSPoint(x: savedX, y: savedY)
            let onAnyScreen = screens.contains { screen in
                screen.frame.insetBy(dx: -40, dy: -40).contains(candidate)
            }
            if onAnyScreen {
                return candidate
            }
        }

        return NSPoint(
            x: visible.maxX - widgetSize.width - 28,
            y: visible.minY + 28
        )
    }

    private func resetPositionToBottomRight() {
        let visible = (NSScreen.main ?? NSScreen.screens.first)?.visibleFrame
            ?? NSRect(x: 0, y: 0, width: 1440, height: 900)
        let origin = NSPoint(
            x: visible.maxX - widgetSize.width - 28,
            y: visible.minY + 28
        )
        panel.setFrameOrigin(origin)
        config.windowX = Double(origin.x)
        config.windowY = Double(origin.y)
        config.save()
    }

    private func applyConfig() {
        panel.alphaValue = CGFloat(config.widgetOpacity)
        widgetView.lockPosition = config.lockPosition
    }

    private func handleFullscreenStateChanged(_ otherFullscreen: Bool) {
        if let settingsWin = settingsController?.window, settingsWin.isVisible {
            return
        }

        if otherFullscreen && !hiddenByFullscreen {
            hiddenByFullscreen = true
            panel.orderOut(nil)
        } else if !otherFullscreen && hiddenByFullscreen {
            hiddenByFullscreen = false
            panel.orderFrontRegardless()
        }
    }

    private func enqueueSilentShred(_ paths: [String]) {
        incrementJobs(delta: 1)
        let passes = config.shredPasses

        shredQueue.async { [weak self] in
            _ = SecureShredder.shredTargets(paths, passCount: passes)
            DispatchQueue.main.async {
                self?.incrementJobs(delta: -1)
            }
        }
    }

    private func incrementJobs(delta: Int) {
        jobsLock.lock()
        pendingJobs = max(0, pendingJobs + delta)
        let active = pendingJobs > 0
        jobsLock.unlock()

        widgetView.isShredding = active
    }

    private func showSettingsWindow() {
        if let existing = settingsController {
            NSApp.activate(ignoringOtherApps: true)
            existing.showWindow(nil)
            existing.window?.makeKeyAndOrderFront(nil)
            return
        }

        let ctrl = SettingsWindowController(config: config) { [weak self] in
            self?.applyConfig()
        }
        settingsController = ctrl
        NSApp.activate(ignoringOtherApps: true)
        ctrl.showWindow(nil)
        ctrl.window?.makeKeyAndOrderFront(nil)
    }
}

// MARK: - Entry Point with Single-Instance Guard

let bundleId = Bundle.main.bundleIdentifier ?? "com.local.desktopshredder"
let runningPeers = NSRunningApplication.runningApplications(withBundleIdentifier: bundleId)
    .filter { $0.processIdentifier != ProcessInfo.processInfo.processIdentifier }

if !runningPeers.isEmpty {
    exit(0)
}

let app = NSApplication.shared
app.setActivationPolicy(.accessory) // Pure desktop floating utility without Dock clutter
let delegate = AppDelegate()
app.delegate = delegate
app.run()
