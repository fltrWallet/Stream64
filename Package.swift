// swift-tools-version: 5.7

import PackageDescription

let package = Package(
    name: "Stream64",
    platforms: [.iOS(.v13), .macOS(.v10_15)],
    products: [
        .library(
            name: "Stream64",
            targets: ["Stream64"]),
    ],
    dependencies: [
        .package(url: "https://github.com/fltrWallet/HaByLo", branch: "main"),
    ],
    targets: [
        .target(
            name: "CStream64",
            dependencies: [],
            sources: [ "CStream64.c", ],
            publicHeadersPath: ".",
            cSettings: [ .headerSearchPath("."), ]),
        .target(
            name: "Stream64",
            dependencies: [ "CStream64",
                            .product(name: "HaByLo", package: "HaByLo"), ]),
        .testTarget(
            name: "Stream64Tests",
            dependencies: ["Stream64"]),
    ]
)
