// OmabarMedia.swift – MediaRemote wrapper
// ─────────────────────────────────────────────────────────────────
// Wraps the private MediaRemote framework to fetch what the system
// media app is currently playing.  The framework symbol is resolved at
// runtime via dlsym so the SDK stays linkable on stock macOS.

import Foundation

/// The media info exposed by the platform's now-playing state.
public struct OmabarMediaInfo: Sendable {
    /// The app owning the session (e.g. "Music", "Spotify").
    public let app: String
    public let title: String
    public let artist: String
    public let album: String
    public let playing: Bool

    public init(app: String, title: String, artist: String, album: String, playing: Bool) {
        self.app = app
        self.title = title
        self.artist = artist
        self.album = album
        self.playing = playing
    }
}

/// Access to now-playing media information via MediaRemote.
public enum OmabarMedia {

    private static let frameworkPath = "/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote"

    // MRMediaRemoteGetNowPlayingInfo(queue, block(NSDictionary))
    private typealias GetNowPlayingFn = @convention(c) (DispatchQueue, @escaping @convention(block) (NSDictionary?) -> Void) -> Void

    private static func load<T>(_ name: String, as _: T.Type) -> T? {
        guard let handle = dlopen(frameworkPath, RTLD_LAZY | RTLD_LOCAL) else { return nil }
        defer { dlclose(handle) }
        guard let sym = dlsym(handle, name) else { return nil }
        return unsafeBitCast(sym, to: T.self)
    }

    private static let kTitle       = "kMRMediaRemoteNowPlayingInfoTitle"
    private static let kArtist      = "kMRMediaRemoteNowPlayingInfoArtist"
    private static let kAlbum       = "kMRMediaRemoteNowPlayingInfoAlbum"
    private static let kPlaybackRate = "kMRMediaRemoteNowPlayingInfoPlaybackRate"
    private static let kAppName     = "kMRMediaRemoteNowPlayingApplicationDisplayNameUserInfoKey"

    /// Fetch the now-playing info synchronously.  Blocks the calling
    /// thread up to `timeout` seconds while MediaRemote responds.
    /// Returns an info struct (with empty strings when no session is
    /// active) or nil when MediaRemote is unavailable.
    public static func getInfo(timeout: TimeInterval = 2.0) -> OmabarMediaInfo? {
        guard let fn = load("MRMediaRemoteGetNowPlayingInfo", as: GetNowPlayingFn.self) else {
            return nil
        }

        let semaphore = DispatchSemaphore(value: 0)
        let holder = ResultHolder()
        let queue = DispatchQueue.global(qos: .userInitiated)
        fn(queue) { dict in
            holder.dict = dict
            semaphore.signal()
        }

        guard semaphore.wait(timeout: .now() + timeout) == .success,
              let dict = holder.dict else {
            return nil
        }

        let str = { (key: String) -> String in
            (dict[key] as? String) ?? ""
        }
        var rate: Float = 0
        if let num = dict[kPlaybackRate] as? NSNumber {
            rate = num.floatValue
        }

        return OmabarMediaInfo(
            app: str(kAppName),
            title: str(kTitle),
            artist: str(kArtist),
            album: str(kAlbum),
            playing: rate > 0.0)
    }

    /// Thread-safe box for the async MediaRemote callback payload.
    private final class ResultHolder: @unchecked Sendable {
        let lock = NSLock()
        private var _dict: NSDictionary?

        var dict: NSDictionary? {
            get { lock.lock(); defer { lock.unlock() }; return _dict }
            set { lock.lock(); _dict = newValue; lock.unlock() }
        }
    }
}