// OmabarVolume.swift – CoreAudio wrapper
// ─────────────────────────────────────────────────────────────────
// Wraps CoreAudio to report the system output volume and mute state.
// Programmatic changes are intentionally not exposed: volume control
// is reserved for the user / menu bar controls.

import Foundation
import CoreAudio

/// Current output audio state.
public struct OmabarVolumeInfo: Sendable {
    /// Output volume in the 0.0…1.0 range (mirrors the scalar property).
    public let volume: Float
    /// Output volume as an integer percentage (0…100).
    public let percentage: Int
    /// Whether the main output element is muted.
    public let muted: Bool

    public init(volume: Float, muted: Bool) {
        self.volume = max(0, min(1, volume))
        self.percentage = Int((self.volume * 100.0).rounded())
        self.muted = muted
    }
}

/// Access to output volume / mute state via CoreAudio.
public enum OmabarVolume {

    private static var defaultOutputDevice: AudioObjectID {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultOutputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var device: AudioObjectID = 0
        var size = UInt32(MemoryLayout<AudioObjectID>.size)
        AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject),
                                   &address, 0, nil, &size, &device)
        return device
    }

    private static func scalarProperty(_ selector: AudioObjectPropertySelector,
                                       _ scope: AudioObjectPropertyScope,
                                       device: AudioObjectID) -> Float? {
        var address = AudioObjectPropertyAddress(
            mSelector: selector,
            mScope: scope,
            mElement: kAudioObjectPropertyElementMain)
        var value: Float = 0
        var size = UInt32(MemoryLayout<Float>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil,
                                         &size, &value) == noErr else { return nil }
        return value
    }

    private static func muteProperty(device: AudioObjectID) -> Bool? {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyMute,
            mScope: kAudioObjectPropertyScopeOutput,
            mElement: kAudioObjectPropertyElementMain)
        var muted: UInt32 = 0
        var size = UInt32(MemoryLayout<UInt32>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil,
                                         &size, &muted) == noErr else { return nil }
        return muted != 0
    }

    /// Current output volume and mute state, or nil when unavailable.
    public static func getInfo() -> OmabarVolumeInfo? {
        let device = defaultOutputDevice
        guard device != 0,
              let volume = scalarProperty(kAudioDevicePropertyVolumeScalar,
                                          kAudioObjectPropertyScopeOutput,
                                          device: device)
        else { return nil }
        let muted = muteProperty(device: device) ?? false
        return OmabarVolumeInfo(volume: volume, muted: muted)
    }

    /// Output volume percentage (0…100), or nil when unavailable.
    public static func volumePercentage() -> Int? {
        getInfo()?.percentage
    }

    /// Whether audio output is muted.
    public static func isMuted() -> Bool {
        getInfo()?.muted ?? false
    }
}