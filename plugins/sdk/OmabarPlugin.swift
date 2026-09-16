// OmabarPlugin.swift – SDK protocol definition for Omabar plugins
// ─────────────────────────────────────────────────────────────────
// Implement the `OmabarPlugin` protocol in your own Swift file.
// The host daemon discovers plugins via SwiftPM / Nix and calls the
// event / query callbacks on a dedicated GCD queue.
//
// See the README in this directory for a getting-started guide.

import Foundation

// MARK: – OmabarEvent (event value + attached data)
//
// Delivered to the plugin's `onEvent` callback.
//
// `kind` distinguishes builtin system events from user-defined custom
// events (e.g. "my_event" from omabar.json).
//
// `data` is the raw JSON value attached by the daemon (e.g.
// "{\"enabled\":true}" for DND updates), or nil for pure system events.

public struct OmabarEvent: Sendable {
    /// Distinguishes builtin system events from user-defined custom events.
    public enum Kind: Sendable {
        case builtin(BuiltinEvent)
        case custom(name: String)
    }

    /// All builtin system event types.  The raw value is the exact wire
    /// name used in subscribe and event-delivery JSON.
    public enum BuiltinEvent: String, Sendable, CaseIterable {
        case volumeChanged      = "volume_changed"
        case powerChanged       = "power_changed"
        case wifiChanged        = "wifi_changed"
        case brightnessChanged  = "brightness_changed"
        case mediaChanged       = "media_changed"
        case frontAppSwitched   = "front_app_switched"
        case spaceChanged       = "space_changed"
        case displayAdded       = "display_added"
        case displayRemoved     = "display_removed"
        case displayMoved       = "display_moved"
        case windowFocused      = "window_focused"
        case scrollTick         = "scroll.tick"
        case animate            = "animate"
        case daemonMessage      = "daemon_message"
        case dnd                = "dnd"
        case routine            = "routine"
        case forced             = "forced"
        case mouseEntered       = "mouse.entered"
        case mouseExited        = "mouse.exited"
        case mouseScrolled      = "mouse.scrolled"
        case mouseClicked       = "mouse.clicked"

        /// All builtin wire names, for subscribing to everything.
        public static let builtinNames: [String] = allCases.map(\.rawValue)
    }

    public let kind: Kind
    /// Raw JSON payload attached by the daemon, if any.
    public let data: String?

    public init(_ kind: Kind, data: String? = nil) {
        self.kind = kind
        self.data = data
    }

    /// Convenience init from a wire name and optional data.
    public init(wireName: String, data: String? = nil) {
        if let builtin = BuiltinEvent(rawValue: wireName) {
            self.kind = .builtin(builtin)
        } else {
            self.kind = .custom(name: wireName)
        }
        self.data = data
    }

    /// The canonical wire name (for subscribe / logging).
    public var wireName: String {
        switch kind {
        case .builtin(let e): return e.rawValue
        case .custom(let n):  return n
        }
    }
}

// MARK: – OmabarQuery / OmabarItemState

/// A query sent from the plugin to the daemon to retrieve the
/// current state of a named bar item.

public struct OmabarQuery: Sendable {
    /// The item name to query (must match a configured item name).
    public let item: String

    public init(item: String) { self.item = item }
}

/// Snapshot of a bar item's current state returned by the daemon.

public struct OmabarItemState: Sendable {
    public let item: String
    public let icon: String
    public let label: String
    /// 32-bit hex ARGB string (e.g. "0xff66ccff") — leading "0x"
    /// is part of the wire format.
    public let background_color: String

    public init(item: String, icon: String, label: String, background_color: String) {
        self.item = item
        self.icon = icon
        self.label = label
        self.background_color = background_color
    }
}

// MARK: – OmabarPlugin protocol

/// Conform to this protocol to create an Omabar plugin.  The host
/// connects via OmabarClient (task 79) and dispatches events on a
/// background queue.
///
/// - `name` — unique plugin name sent to the daemon on registration.
/// - `eventNames` — events to subscribe to, e.g.
///   `[OmabarEvent(.builtin(.dnd)), OmabarEvent(.builtin(.volumeChanged))]`
/// - `onEvent` — called for every matching event.
/// - `onQuery` — called when the plugin requests a bar item's state;
///   return the snapshot, or nil if the item does not exist.
public protocol OmabarPlugin: Sendable {
    /// Unique plugin name (sent to the daemon on registration).
    var name: String { get }

    /// Events the plugin subscribes to.
    var eventNames: [OmabarEvent] { get }

    /// Called when a subscribed event fires.
    func onEvent(_ event: OmabarEvent) async

    /// Called when the plugin queries a bar item; return its current
    /// state, or nil if the item does not exist.
    func onQuery(_ query: OmabarQuery) async -> OmabarItemState?
}
