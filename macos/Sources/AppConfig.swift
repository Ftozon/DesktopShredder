import Foundation

/// Strictly offline local configuration stored at:
/// ~/Library/Application Support/DesktopShredder/settings.json
/// Never logs or persists any shredded file paths or history.
final class AppConfig: Codable {
    var windowX: Double?
    var windowY: Double?
    var shredPasses: Int = 7 // Default: 7-pass DoD 5220.22-M ECE + CSPRNG
    var widgetOpacity: Double = 0.95
    var lockPosition: Bool = false

    private static var configURL: URL {
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
            ?? URL(fileURLWithPath: NSHomeDirectory()).appendingPathComponent("Library/Application Support")
        return appSupport
            .appendingPathComponent("DesktopShredder", isDirectory: true)
            .appendingPathComponent("settings.json", isDirectory: false)
    }

    static func load() -> AppConfig {
        let url = configURL
        guard let data = try? Data(contentsOf: url),
              let decoded = try? JSONDecoder().decode(AppConfig.self, from: data) else {
            return AppConfig()
        }
        decoded.shredPasses = Self.normalizePasses(decoded.shredPasses)
        decoded.widgetOpacity = min(max(decoded.widgetOpacity, 0.40), 1.0)
        return decoded
    }

    func save() {
        shredPasses = Self.normalizePasses(shredPasses)
        widgetOpacity = min(max(widgetOpacity, 0.40), 1.0)

        let url = Self.configURL
        let dir = url.deletingLastPathComponent()
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)

        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        if let data = try? encoder.encode(self) {
            try? data.write(to: url, options: .atomic)
        }
    }

    static func normalizePasses(_ passes: Int) -> Int {
        if passes >= 35 { return 35 }
        if passes <= 3 { return 3 }
        return 7
    }
}
