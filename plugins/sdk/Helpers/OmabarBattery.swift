// OmabarBattery.swift – IOKit power-sources wrapper
// ─────────────────────────────────────────────────────────────────
// Mirrors the daemon's power.c: total/average capacity across all
// power sources, `charging` = plugged in OR a battery finishing its
// charge cycle, `pluggedIn` from IOPSGetProvidingPowerSourceType.

import Foundation
import IOKit.ps

/// Battery / power-source state.
public struct OmabarBatteryInfo: Sendable {
    /// Charge percentage (0…100), or nil when no capacity is reported.
    public let percentage: Int?
    /// Whether the battery is charging (incl. plugged-in finishing cycle).
    public let charging: Bool
    /// Whether the machine is plugged into external power.
    public let pluggedIn: Bool

    public init(percentage: Int?, charging: Bool, pluggedIn: Bool) {
        self.percentage = percentage
        self.charging = charging
        self.pluggedIn = pluggedIn
    }
}

/// Access to battery / power-source information via IOKit.
public enum OmabarBattery {

    private static func capacityInfo() -> (total: Int, count: Int) {
        guard let info = IOPSCopyPowerSourcesInfo()?.takeRetainedValue(),
              let list = IOPSCopyPowerSourcesList(info)?.takeRetainedValue() as? [CFTypeRef]
        else { return (0, 0) }

        var total = 0
        var count = 0
        for source in list {
            guard let desc = IOPSGetPowerSourceDescription(info, source)?
                .takeUnretainedValue() as NSDictionary? else { continue }
            if let cap = desc[kIOPSCurrentCapacityKey] as? NSNumber {
                total += cap.intValue
                count += 1
            }
        }
        return (total, count)
    }

    /// Battery / power status.
    public static func getInfo() -> OmabarBatteryInfo {
        let (total, count) = capacityInfo()
        let percentage = count > 0 ? max(0, min(100, total / count)) : nil

        // plugged in? -> IOPSGetProvidingPowerSourceType
        var pluggedIn = false
        if let info = IOPSCopyPowerSourcesInfo()?.takeRetainedValue() {
            let type: CFString? = IOPSGetProvidingPowerSourceType(info)?
                .takeUnretainedValue()
            if let type = type as String? {
                pluggedIn = (type == kIOPSACPowerValue as String
                             || type == kIOPMUPSPowerKey as String
                             || type == kIOPMACPowerKey as String)
            }
        }

        // charging -> plugged in OR any present battery finishing cycle
        var charging = pluggedIn
        if let info = IOPSCopyPowerSourcesInfo()?.takeRetainedValue(),
           let list = IOPSCopyPowerSourcesList(info)?.takeRetainedValue() as? [CFTypeRef] {
            for source in list {
                guard let desc = IOPSGetPowerSourceDescription(info, source)?
                    .takeUnretainedValue() as NSDictionary? else { continue }
                guard (desc[kIOPSIsPresentKey] as? Bool) == true else { continue }
                let state = desc[kIOPSPowerSourceStateKey] as? String ?? ""
                if state != kIOPSBatteryPowerValue as String { charging = true; break }
            }
        }

        return OmabarBatteryInfo(percentage: percentage,
                                 charging: charging,
                                 pluggedIn: pluggedIn)
    }

    /// Charge percentage (0…100) — nil for machines without a battery.
    public static func percentage() -> Int? {
        getInfo().percentage
    }

    /// Whether the battery exists and is currently charging.
    public static func isCharging() -> Bool {
        getInfo().charging
    }

    /// Whether the machine is plugged into external power.
    public static func isPluggedIn() -> Bool {
        getInfo().pluggedIn
    }
}