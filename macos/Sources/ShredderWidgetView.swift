import AppKit
import Foundation

/// Floating non-activating desktop panel that stays on all desktop spaces without stealing keyboard focus.
final class ShredderPanel: NSPanel {
    init(contentRect: NSRect) {
        super.init(
            contentRect: contentRect,
            styleMask: [.borderless, .nonactivatingPanel],
            backing: .buffered,
            defer: false
        )
        isOpaque = false
        backgroundColor = .clear
        hasShadow = true
        level = .floating
        // Deliberately omit .fullScreenAuxiliary so macOS automatically keeps it out of FullScreen Spaces
        collectionBehavior = [.canJoinAllSpaces, .stationary, .ignoresCycle]
        isMovableByWindowBackground = false
        hidesOnDeactivate = false
    }

    override var canBecomeKey: Bool { false }
    override var canBecomeMain: Bool { false }
}

/// Vector Retina Paper Shredder view supporting silent drag-and-drop shredding, dragging, and context menu.
final class ShredderWidgetView: NSView {
    var isDragHovering: Bool = false {
        didSet { needsDisplay = true }
    }

    var isShredding: Bool = false {
        didSet {
            if isShredding {
                startAnimation()
            } else {
                stopAnimation()
            }
            needsDisplay = true
        }
    }

    var lockPosition: Bool = false
    var onFilesDropped: (([String]) -> Void)?
    var onPositionChanged: ((NSPoint) -> Void)?
    var onOpenSettings: (() -> Void)?
    var onResetPosition: (() -> Void)?
    var onQuit: (() -> Void)?

    private var animTimer: Timer?
    private var animPhase: CGFloat = 0
    private var initialMouseScreenLoc: NSPoint = .zero
    private var initialWindowOrigin: NSPoint = .zero
    private var isDraggingWindow: Bool = false

    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        registerForDraggedTypes([.fileURL])
        toolTip = "拖入文件或文件夹即可静默彻底粉碎（右键打开设置）"
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override var isFlipped: Bool { true }

    override func acceptsFirstMouse(for event: NSEvent?) -> Bool {
        return true
    }

    // MARK: - Animation

    private func startAnimation() {
        guard animTimer == nil else { return }
        animPhase = 0
        let t = Timer(timeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            self.animPhase += 0.18
            self.needsDisplay = true
        }
        RunLoop.main.add(t, forMode: .common)
        animTimer = t
    }

    private func stopAnimation() {
        animTimer?.invalidate()
        animTimer = nil
        animPhase = 0
    }

    // MARK: - Dragging Floating Widget on Desktop

    override func mouseDown(with event: NSEvent) {
        guard !lockPosition, let win = window else { return }
        isDraggingWindow = true
        initialMouseScreenLoc = NSEvent.mouseLocation
        initialWindowOrigin = win.frame.origin
    }

    override func mouseDragged(with event: NSEvent) {
        guard isDraggingWindow, !lockPosition, let win = window else { return }
        let currentLoc = NSEvent.mouseLocation
        let dx = currentLoc.x - initialMouseScreenLoc.x
        let dy = currentLoc.y - initialMouseScreenLoc.y
        let nextOrigin = NSPoint(x: initialWindowOrigin.x + dx, y: initialWindowOrigin.y + dy)
        win.setFrameOrigin(nextOrigin)
    }

    override func mouseUp(with event: NSEvent) {
        guard isDraggingWindow, let win = window else { return }
        isDraggingWindow = false
        onPositionChanged?(win.frame.origin)
    }

    // MARK: - Context Menu (Right Click)

    override func menu(for event: NSEvent) -> NSMenu? {
        let menu = NSMenu(title: "桌面碎纸机")
        let settingsItem = NSMenuItem(title: "设置 (Settings)...", action: #selector(handleOpenSettings), keyEquivalent: "")
        settingsItem.target = self
        menu.addItem(settingsItem)

        let resetItem = NSMenuItem(title: "重置位置到右下角", action: #selector(handleResetPosition), keyEquivalent: "")
        resetItem.target = self
        menu.addItem(resetItem)

        menu.addItem(NSMenuItem.separator())

        let quitItem = NSMenuItem(title: "退出碎纸机", action: #selector(handleQuit), keyEquivalent: "")
        quitItem.target = self
        menu.addItem(quitItem)
        return menu
    }

    @objc private func handleOpenSettings() {
        onOpenSettings?()
    }

    @objc private func handleResetPosition() {
        onResetPosition?()
    }

    @objc private func handleQuit() {
        onQuit?()
    }

    // MARK: - Drag & Drop File Shredding

    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        if hasValidFileURLs(sender) {
            isDragHovering = true
            return .generic
        }
        return []
    }

    override func draggingUpdated(_ sender: NSDraggingInfo) -> NSDragOperation {
        return hasValidFileURLs(sender) ? .generic : []
    }

    override func draggingExited(_ sender: NSDraggingInfo?) {
        isDragHovering = false
    }

    override func prepareForDragOperation(_ sender: NSDraggingInfo) -> Bool {
        return hasValidFileURLs(sender)
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        isDragHovering = false
        guard let urls = sender.draggingPasteboard.readObjects(
            forClasses: [NSURL.self],
            options: [.urlReadingFileURLsOnly: true]
        ) as? [URL], !urls.isEmpty else {
            return false
        }

        let paths = urls.map { $0.path }.filter { !$0.isEmpty }
        guard !paths.isEmpty else { return false }

        // Silent immediate destruction without confirmation modal
        onFilesDropped?(paths)
        return true
    }

    private func hasValidFileURLs(_ sender: NSDraggingInfo) -> Bool {
        return sender.draggingPasteboard.canReadObject(
            forClasses: [NSURL.self],
            options: [.urlReadingFileURLsOnly: true]
        )
    }

    // MARK: - Vector Drawing (Matches Shredder Icon Design)

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        guard let ctx = NSGraphicsContext.current?.cgContext else { return }
        ctx.saveGState()
        defer { ctx.restoreGState() }

        // 1. Paper Sheet Feeding Into Top Slot (clipped above cutter head)
        NSGraphicsContext.saveGraphicsState()
        let paperClip = NSBezierPath(rect: NSRect(x: 24, y: 2, width: 48, height: 34))
        paperClip.addClip()

        let paperOffset: CGFloat = isShredding ? ((animPhase * 5.0).truncatingRemainder(dividingBy: 22.0)) : 0.0
        let paperRect = NSRect(x: 26, y: 6 + paperOffset, width: 44, height: 34)
        let paperPath = NSBezierPath(roundedRect: paperRect, xRadius: 3, yRadius: 3)
        NSColor(srgbRed: 0.973, green: 0.980, blue: 0.988, alpha: 1.0).setFill()
        paperPath.fill()
        NSColor(srgbRed: 0.796, green: 0.835, blue: 0.882, alpha: 1.0).setStroke()
        paperPath.lineWidth = 1.0
        paperPath.stroke()

        // Document mock lines
        let redLine = NSBezierPath(roundedRect: NSRect(x: 33, y: 13 + paperOffset, width: 22, height: 3), xRadius: 1.5, yRadius: 1.5)
        NSColor(srgbRed: 0.937, green: 0.267, blue: 0.267, alpha: 0.9).setFill()
        redLine.fill()

        let grayLine1 = NSBezierPath(roundedRect: NSRect(x: 33, y: 19.5 + paperOffset, width: 30, height: 2.5), xRadius: 1.2, yRadius: 1.2)
        NSColor(srgbRed: 0.580, green: 0.639, blue: 0.722, alpha: 0.9).setFill()
        grayLine1.fill()

        let grayLine2 = NSBezierPath(roundedRect: NSRect(x: 33, y: 25.5 + paperOffset, width: 26, height: 2.5), xRadius: 1.2, yRadius: 1.2)
        grayLine2.fill()
        NSGraphicsContext.restoreGraphicsState()

        // 2. Lower Shredder Bin Body
        let binRect = NSRect(x: 14, y: 46, width: 68, height: 58)
        let binPath = NSBezierPath(roundedRect: binRect, xRadius: 9, yRadius: 9)
        NSColor(srgbRed: 0.118, green: 0.161, blue: 0.231, alpha: 1.0).setFill()
        binPath.fill()
        NSColor(srgbRed: 0.200, green: 0.255, blue: 0.333, alpha: 1.0).setStroke()
        binPath.lineWidth = 1.2
        binPath.stroke()

        // Viewing Window on Front of Bin
        let winRect = NSRect(x: 23, y: 58, width: 50, height: 37)
        let winPath = NSBezierPath(roundedRect: winRect, xRadius: 5, yRadius: 5)
        NSColor(srgbRed: 0.059, green: 0.090, blue: 0.165, alpha: 1.0).setFill()
        winPath.fill()

        // Shredded Paper Strips inside Viewing Window
        NSGraphicsContext.saveGraphicsState()
        winPath.addClip()
        let baseHeights: [CGFloat] = [21, 28, 17, 30, 23, 26]
        for i in 0..<6 {
            let sx = CGFloat(28 + i * 7)
            let wave = isShredding ? (5.0 * sin(animPhase * 2.6 + CGFloat(i) * 1.15)) : 0.0
            let h = min(max(baseHeights[i] + wave, 10), 33)
            let stripRect = NSRect(x: sx, y: 60, width: 4.5, height: h)
            let stripPath = NSBezierPath(roundedRect: stripRect, xRadius: 1.0, yRadius: 1.0)
            if i == 3 {
                NSColor(srgbRed: 0.996, green: 0.792, blue: 0.792, alpha: 0.95).setFill()
            } else {
                NSColor(srgbRed: 0.910, green: 0.933, blue: 0.961, alpha: 0.95).setFill()
            }
            stripPath.fill()
        }
        NSGraphicsContext.restoreGraphicsState()

        NSColor(srgbRed: 0.278, green: 0.333, blue: 0.412, alpha: 1.0).setStroke()
        winPath.lineWidth = 1.0
        winPath.stroke()

        // 3. Shredder Cutter Head / Metallic Cap
        let headRect = NSRect(x: 10, y: 31, width: 76, height: 22)
        let headPath = NSBezierPath(roundedRect: headRect, xRadius: 6.5, yRadius: 6.5)
        if isDragHovering {
            NSColor(srgbRed: 0.200, green: 0.255, blue: 0.333, alpha: 1.0).setFill()
        } else {
            NSColor(srgbRed: 0.278, green: 0.333, blue: 0.412, alpha: 1.0).setFill()
        }
        headPath.fill()

        // Top metallic bevel band
        let topBand = NSBezierPath(roundedRect: NSRect(x: 10, y: 31, width: 76, height: 10), xRadius: 6.5, yRadius: 6.5)
        NSColor(srgbRed: 0.392, green: 0.455, blue: 0.545, alpha: 0.75).setFill()
        topBand.fill()

        NSColor(srgbRed: 0.45, green: 0.52, blue: 0.61, alpha: 1.0).setStroke()
        headPath.lineWidth = 1.0
        headPath.stroke()

        // Cutter Throat Slot
        let slotRect = NSRect(x: 19, y: 35, width: 58, height: 5.5)
        let slotPath = NSBezierPath(roundedRect: slotRect, xRadius: 2.5, yRadius: 2.5)
        if isDragHovering {
            NSColor(srgbRed: 0.863, green: 0.149, blue: 0.149, alpha: 1.0).setFill()
        } else {
            NSColor(srgbRed: 0.059, green: 0.090, blue: 0.165, alpha: 1.0).setFill()
        }
        slotPath.fill()

        // Crimson Danger Accent Bar
        let accentPath = NSBezierPath(roundedRect: NSRect(x: 20, y: 45.5, width: 42, height: 2.5), xRadius: 1.2, yRadius: 1.2)
        NSColor(srgbRed: 0.937, green: 0.267, blue: 0.267, alpha: 0.95).setFill()
        accentPath.fill()

        // Status LED
        let ledRect = NSRect(x: 69, y: 43.5, width: 6.5, height: 6.5)
        let ledPath = NSBezierPath(ovalIn: ledRect)
        if isShredding {
            let pulse = CGFloat(0.55 + 0.45 * abs(sin(animPhase * 2.0)))
            NSColor(srgbRed: 0.937, green: 0.267, blue: 0.267, alpha: pulse).setFill()
        } else if isDragHovering {
            NSColor(srgbRed: 0.961, green: 0.620, blue: 0.043, alpha: 1.0).setFill()
        } else {
            NSColor(srgbRed: 0.133, green: 0.773, blue: 0.369, alpha: 1.0).setFill()
        }
        ledPath.fill()
    }
}
