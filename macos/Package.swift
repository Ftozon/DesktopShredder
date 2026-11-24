// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "DesktopShredder",
    platforms: [
        .macOS(.v12)
    ],
    products: [
        .executable(name: "DesktopShredder", targets: ["DesktopShredder"])
    ],
    targets: [
        .executableTarget(
            name: "DesktopShredder",
            path: "Sources",
            linkerSettings: [
                .linkedFramework("AppKit"),
                .linkedFramework("CoreGraphics"),
                .linkedFramework("Security")
            ]
        )
    ]
)
