// plugins/media/main.swift
// ─────────────────────────────────────────────────────────────────
// Example omabar plugin: shows the currently-playing track via
// MediaRemote and pushes updates whenever playback changes.  Rich
// item updates: label = "<title> — <artist>" · "· <album>".
//
// Requires a configured bar item named "media" in the daemon config.
//
// Build:
//   swiftc -o media OmabarPlugin.swift OmabarClient.swift \
//     Helpers/OmabarMedia.swift main.swift

import Foundation

let itemName = "media"
let fallbackPollInterval: TimeInterval = 10   // seconds

let client = OmabarClient()

do {
    try client.connect()
} catch {
    print("media: failed to connect to daemon: \(error)")
    exit(2)
}

// 1. Register with the daemon.
do {
    let ok = try await client.send(OmabarMessage.register(name: "media"))
    if !ok { print("media: daemon rejected registration") }
} catch {
    print("media: register failed: \(error)")
    client.disconnect()
    exit(2)
}

// 2. Subscribe to now-playing pushes from the daemon (and a few others
//    we might as well refresh on).
do {
    let ok = try await client.subscribe(to: ["media_changed", "volume_changed", "dnd", "routine"])
    if !ok { print("media: subscribe rejected") }
} catch {
    print("media: subscribe failed: \(error)")
}

// 3. Republish on any event we are subscribed to.
client.eventHandler = { event in
    await sendUpdate(client: client)
}

// 4. Fallback poll: MediaRemote may not push through the daemon in
//    every environment, so also refresh periodically.
while true {
    await sendUpdate(client: client)
    try? await Task.sleep(nanoseconds: UInt64(fallbackPollInterval * 1_000_000_000))
    if Task.isCancelled { break }
}

client.disconnect()

// ── Update helpers ───────────────────────────────────────────────

/// Read now-playing info and push an update to the daemon.
func sendUpdate(client: OmabarClient) async {
    let info = OmabarMedia.getInfo(timeout: 1.0)
    let (icon, label) = describe(info)
    do {
        let ok = try await client.update(item: itemName, icon: icon, label: label, backgroundColor: nil)
        if !ok { print("media: update rejected (is an item named '\(itemName)' configured?)") }
    } catch {
        print("media: update failed: \(error)")
    }
}

/// Map now-playing info to icon + label.
func describe(_ info: OmabarMediaInfo?) -> (icon: String, label: String) {
    guard let info, !info.title.isEmpty else {
        return (icon: "\u{f144}", label: "no media")   // 󰅄 music-note
    }
    let artist = info.artist
    let album = info.album
    var label = info.title
    if !artist.isEmpty { label += " — \(artist)" }
    if !album.isEmpty && !artist.isEmpty { label += " · \(album)" }
    let icon = info.playing ? "\u{f144}" : "\u{f04b}"   // 󰅄 playing, 󰁋 paused
    return (icon: icon, label: label)
}