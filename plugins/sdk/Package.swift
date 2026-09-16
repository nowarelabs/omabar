// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "Omabar",
    platforms: [.macOS(.v13)],
    products: [
        .library(name: "Omabar", targets: ["Omabar"])
    ],
    targets: [
        .target(
            name: "Omabar",
            path: ".",
            exclude: ["Package.swift"],
            sources: [
                "OmabarPlugin.swift",
                "OmabarClient.swift",
                "Helpers/OmabarBrightness.swift",
                "Helpers/OmabarMedia.swift",
                "Helpers/OmabarNetwork.swift",
                "Helpers/OmabarVolume.swift",
                "Helpers/OmabarBattery.swift"
            ]
        )
    ]
)