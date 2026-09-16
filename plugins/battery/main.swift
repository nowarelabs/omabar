// plugins/battery/main.swift
// ─────────────────────────────────────────────────────────────────
// Example omabar plugin: monitors the battery via IOKit and pushes
// periodic updates to the daemon.  Demonstrates the full plugin
// lifecycle — connect, register, subscribe, periodic timer, update
// item.
//
// Requires a configured bar item named "battery" in the daemon config
// (the plugin updates that item's icon/label), e.g.:
//   items.battery = { type = "plain"; ... }
//
// Build:
//   swiftc -o battery OmabarPlugin.swift OmabarClient.swift \
//     Helpers/OmabarBattery.swift main.swift

import Foundation

let itemName = "battery"
let interval: TimeInterval = 30   // seconds

let client = OmabarClient()

do {
    try client.connect()
} catch {
    print("battery: failed to connect to daemon: \(error)")
    exit(2)
}

// 1. Register with the daemon.
do {
    let ok = try await client.send(OmabarMessage.register(name: "battery"))
    if !ok { print("battery: daemon rejected registration") }
} catch {
    print("battery: register failed: \(error)")
    client.disconnect()
    exit(2)
}

// 2. Subscribe to battery/power events pushed by the daemon.
do {
    let ok = try await client.subscribe(to: ["power_changed", "dnd", "routine"])
    if !ok { print("battery: subscribe rejected") }
} catch {
    print("battery: subscribe failed: \(error)")
}

// 3. Event handler: push a fresh reading whenever an event fires.
client.eventHandler = { event in
    await sendUpdate(client: client)
}

// 4. Periodic timer: keep the item fresh even without events.
while true {
    await sendUpdate(client: client)
    try? await Task.sleep(nanoseconds: UInt64(interval * 1_000_000_000))
    if Task.isCancelled { break }
}

client.disconnect()

// ── Update helpers ───────────────────────────────────────────────

/// Read battery state and push an update to the daemon.
func sendUpdate(client: OmabarClient) async {
    let info = OmabarBattery.getInfo()
    let (icon, label, bg) = describe(info)
    do {
        let ok = try await client.update(item: itemName, icon: icon, label: label, backgroundColor: bg)
        if !ok { print("battery: update rejected (is an item named '\(itemName)' configured?)") }
    } catch {
        print("battery: update failed: \(error)")
    }
}

/// Map a battery reading to icon + label + optional background color.
/// Nerd-Font style battery glyphs; falls back to a plain circle
/// sequence when battery info is unavailable.
func describe(_ info: OmabarBatteryInfo) -> (icon: String, label: String, bg: String?) {
    guard let pct = info.percentage else {
        return (icon: "\u{f0e7}", label: "N/A", bg: "0x00000000")
    }
    let glyph: String
    switch pct {
    case 100:     glyph = "\u{f0b9}"   // 󰂹 full
    case 90...99: glyph = "\u{f0c2}"   // 󰂂
    case 80...89: glyph = "\u{f0c1}"   // 󰂁
    case 70...79: glyph = "\u{f0c0}"   // 󰂀
    case 60...69: glyph = "\u{f0bf}"   // 󰁿
    case 50...59: glyph = "\u{f0be}"   // 󰁾
    case 40...49: glyph = "\u{f0bd}"   // 󰁽
    case 30...39: glyph = "\u{f0bc}"   // 󰁼
    case 20...29: glyph = "\u{f0bb}"   // 󰁻
    default:      glyph = "\u{f0ba}"   // 󰁺 empty
    }
    let chargingGlyph = info.charging ? "\u{f0c4}" : ""   // 󰂄 bolt
    let icon = "\(chargingGlyph)\(glyph)"

    var label = "\(pct)%"
    if info.charging { label += " ⚡" }
    let bg: String?
    if pct < 25 {
        bg = "0xffcc3232"   // red when low
    } else {
        bg = nil
    }
    return (icon: icon, label: label, bg: bg)
}