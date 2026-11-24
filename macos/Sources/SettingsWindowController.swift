import AppKit
import Foundation

/// Native AppKit Settings window for configuring startup, position lock, overwrite passes, and opacity.
final class SettingsWindowController: NSWindowController {
    private let config: AppConfig
    private let onSaveCallback: () -> Void

    private let autoStartCheck = NSButton(checkboxWithTitle: "开机自动启动（登录 macOS 时自动悬浮于桌面）", target: nil, action: nil)
    private let lockPosCheck = NSButton(checkboxWithTitle: "锁定桌面悬浮窗位置（防止鼠标误拖动）", target: nil, action: nil)
    private let passesPopUp = NSPopUpButton(frame: .zero, pullsDown: false)
    private let opacitySlider = NSSlider(value: 0.95, minValue: 0.40, maxValue: 1.00, target: nil, action: nil)
    private let opacityLabel = NSTextField(labelWithString: "悬浮窗不透明度：95%")

    init(config: AppConfig, onSave: @escaping () -> Void) {
        self.config = config
        self.onSaveCallback = onSave

        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 430, height: 300),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.title = "桌面碎纸机 - 设置"
        win.isReleasedWhenClosed = false
        win.center()

        super.init(window: win)
        buildUI()
        populateValues()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func buildUI() {
        guard let contentView = window?.contentView else { return }

        let titleLabel = NSTextField(labelWithString: "桌面碎纸机设置")
        titleLabel.font = NSFont.boldSystemFont(ofSize: 16)

        let subtitleLabel = NSTextField(labelWithString: "纯本地离线运行 · 免登录 · 零日志静默销毁")
        subtitleLabel.font = NSFont.systemFont(ofSize: 12)
        subtitleLabel.textColor = .secondaryLabelColor

        let passesTitle = NSTextField(labelWithString: "安全粉碎覆写标准（拖入即静默执行，不可恢复）：")
        passesTitle.font = NSFont.systemFont(ofSize: 12.5)

        passesPopUp.addItems(withTitles: [
            "7 次覆写 — DoD 5220.22-M ECE + CSPRNG（默认推荐）",
            "3 次覆写 — DoD 5220.22-M 标准（兼顾大文件速度）",
            "35 次覆写 — Gutmann 极限覆写标准（适合高敏小文件）"
        ])

        opacitySlider.target = self
        opacitySlider.action = #selector(opacityChanged)

        let cancelBtn = NSButton(title: "取消", target: self, action: #selector(cancelClicked))
        cancelBtn.keyEquivalent = "\u{1b}"

        let saveBtn = NSButton(title: "保存设置", target: self, action: #selector(saveClicked))
        saveBtn.keyEquivalent = "\r"
        saveBtn.bezelStyle = .rounded

        let buttonRow = NSStackView(views: [cancelBtn, saveBtn])
        buttonRow.orientation = .horizontal
        buttonRow.spacing = 10

        let stack = NSStackView(views: [
            titleLabel,
            subtitleLabel,
            autoStartCheck,
            lockPosCheck,
            passesTitle,
            passesPopUp,
            opacityLabel,
            opacitySlider,
            buttonRow
        ])
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.spacing = 11
        stack.translatesAutoresizingMaskIntoConstraints = false

        contentView.addSubview(stack)
        NSLayoutConstraint.activate([
            stack.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 22),
            stack.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -22),
            stack.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 20),
            passesPopUp.widthAnchor.constraint(equalTo: stack.widthAnchor),
            opacitySlider.widthAnchor.constraint(equalTo: stack.widthAnchor),
            buttonRow.trailingAnchor.constraint(equalTo: stack.trailingAnchor)
        ])
    }

    private func populateValues() {
        autoStartCheck.state = StartupManager.isAutoStartEnabled() ? .on : .off
        lockPosCheck.state = config.lockPosition ? .on : .off

        if config.shredPasses >= 35 {
            passesPopUp.selectItem(at: 2)
        } else if config.shredPasses <= 3 {
            passesPopUp.selectItem(at: 1)
        } else {
            passesPopUp.selectItem(at: 0)
        }

        opacitySlider.doubleValue = config.widgetOpacity
        updateOpacityText()
    }

    @objc private func opacityChanged() {
        updateOpacityText()
    }

    private func updateOpacityText() {
        let pct = Int((opacitySlider.doubleValue * 100.0).rounded())
        opacityLabel.stringValue = "悬浮窗不透明度：\(pct)%"
    }

    @objc private func saveClicked() {
        StartupManager.setAutoStart(autoStartCheck.state == .on)
        config.lockPosition = (lockPosCheck.state == .on)
        config.widgetOpacity = min(max(opacitySlider.doubleValue, 0.40), 1.0)

        switch passesPopUp.indexOfSelectedItem {
        case 1:
            config.shredPasses = 3
        case 2:
            config.shredPasses = 35
        default:
            config.shredPasses = 7
        }

        config.save()
        onSaveCallback()
        close()
    }

    @objc private func cancelClicked() {
        close()
    }
}
