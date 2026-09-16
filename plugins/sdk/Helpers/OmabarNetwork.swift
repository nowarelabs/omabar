// OmabarNetwork.swift – CoreWLAN wrapper
// ─────────────────────────────────────────────────────────────────
// Wraps CoreWLAN to report the currently associated Wi-Fi network and
// link information for the primary interface.  No network permission
// prompt is required to read the SSID of the current network.

import Foundation
import CoreWLAN

/// Information about the active Wi-Fi connection.
public struct OmabarNetworkInfo: Sendable {
    /// The network name (SSID), or nil when not on Wi-Fi.
    public let ssid: String?
    /// dBm signal strength of the primary interface, if available.
    public let rssi: Int?
    /// Basic Service Set ID (MAC) of the access point, if available.
    public let bssid: String?

    public init(ssid: String?, rssi: Int?, bssid: String?) {
        self.ssid = ssid
        self.rssi = rssi
        self.bssid = bssid
    }
}

/// Access to network / Wi-Fi status via CoreWLAN.
public enum OmabarNetwork {

    /// The active network info synchronously.  Returns nil when Wi-Fi
    /// is off or the interface is not associated.
    public static func getInfo() -> OmabarNetworkInfo? {
        guard let interface = CWWiFiClient.shared().interface() else { return nil }
        let ssid = interface.ssid()
        if ssid == nil && interface.rssiValue() == 0 && interface.bssid() == nil { return nil }
        return OmabarNetworkInfo(ssid: ssid,
                                 rssi: interface.rssiValue(),
                                 bssid: interface.bssid())
    }

    /// The current SSID string, or nil when off / not associated.
    public static func ssid() -> String? {
        getInfo()?.ssid
    }

    /// Whether the machine is associated with a Wi-Fi network.
    public static func isConnected() -> Bool {
        ssid() != nil
    }
}