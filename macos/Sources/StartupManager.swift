import Foundation

/// Manages macOS Launch-at-Login via ~/Library/LaunchAgents/com.local.desktopshredder.plist.
/// Works reliably for standalone local .app bundles without requiring paid Apple Developer ID notarization.
enum StartupManager {
    private static let agentLabel = "com.local.desktopshredder"

    private static var launchAgentURL: URL {
        let home = FileManager.default.homeDirectoryForCurrentUser
        return home
            .appendingPathComponent("Library/LaunchAgents", isDirectory: true)
            .appendingPathComponent("\(agentLabel).plist", isDirectory: false)
    }

    static func isAutoStartEnabled() -> Bool {
        return FileManager.default.fileExists(atPath: launchAgentURL.path)
    }

    @discardableResult
    static func setAutoStart(_ enabled: Bool) -> Bool {
        let plistURL = launchAgentURL
        if !enabled {
            if FileManager.default.fileExists(atPath: plistURL.path) {
                do {
                    try FileManager.default.removeItem(at: plistURL)
                } catch {
                    return false
                }
            }
            return true
        }

        guard let execPath = Bundle.main.executablePath ?? CommandLine.arguments.first else {
            return false
        }
        let resolvedExecPath = URL(fileURLWithPath: execPath).standardizedFileURL.path

        let plistDict: [String: Any] = [
            "Label": agentLabel,
            "ProgramArguments": [resolvedExecPath],
            "RunAtLoad": true,
            "KeepAlive": false,
            "ProcessType": "Interactive"
        ]

        do {
            let dir = plistURL.deletingLastPathComponent()
            try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
            let plistData = try PropertyListSerialization.data(
                fromPropertyList: plistDict,
                format: .xml,
                options: 0
            )
            try plistData.write(to: plistURL, options: .atomic)
            return true
        } catch {
            return false
        }
    }
}
