// OmabarBrightness.swift – DisplayServices wrapper
// ─────────────────────────────────────────────────────────────────
// Wraps the private DisplayServices framework for reading and (where
// permitted) setting display brightness.  Symbols are resolved at
// runtime via dlsym so the SDK stays linkable on stock macOS.

import Foundation
import CoreGraphics

/// Access to display brightness through DisplayServices.
///
/// All calls are safe when the underlying symbol is unavailable
/// (e.g. on machines without controllable brightness): the getters
/// return `nil` and `canChange` returns `false`.
public enum OmabarBrightness {

    private typealias GetBrightnessFn = @convention(c) (CGDirectDisplayID, UnsafeMutablePointer<Float>) -> Int32
    private typealias CanChangeFn = @convention(c) (CGDirectDisplayID) -> Int32
    private typealias SetBrightnessFn = @convention(c) (CGDirectDisplayID, Float) -> Int32

    private static let lib = "/System/Library/PrivateFrameworks/DisplayServices.framework/DisplayServices"

    private static func load<T>(_ name: String, as _: T.Type) -> T? {
        guard let handle = dlopen(lib, RTLD_LAZY | RTLD_LOCAL) else { return nil }
        defer { dlclose(handle) }
        guard let sym = dlsym(handle, name) else { return nil }
        return unsafeBitCast(sym, to: T.self)
    }

    /// The current brightness of `display` in the 0.0…1.0 range,
    /// or nil if unavailable.
    public static func brightness(of display: CGDirectDisplayID = CGMainDisplayID()) -> Float? {
        guard let fn = load("DisplayServicesGetBrightness", as: GetBrightnessFn.self) else { return nil }
        var value: Float = 0
        guard fn(display, &value) == noErr else { return nil }
        return value
    }

    /// Current brightness as an integer percentage (0…100).
    public static func brightnessPercentage(of display: CGDirectDisplayID = CGMainDisplayID()) -> Int? {
        guard let b = brightness(of: display) else { return nil }
        return Int((b * 100.0).rounded())
    }

    /// Whether the display brightness can be changed programmatically.
    public static func canChangeBrightness(of display: CGDirectDisplayID = CGMainDisplayID()) -> Bool {
        guard let fn = load("DisplayServicesCanChangeBrightness", as: CanChangeFn.self) else { return false }
        return fn(display) == noErr
    }

    /// Set display brightness (0.0…1.0).  Returns false if the
    /// operation is unsupported or failed.
    @discardableResult
    public static func setBrightness(_ value: Float, on display: CGDirectDisplayID = CGMainDisplayID()) -> Bool {
        guard let fn = load("DisplayServicesSetBrightness", as: SetBrightnessFn.self) else { return false }
        return fn(display, max(0, min(1, value))) == noErr
    }
}