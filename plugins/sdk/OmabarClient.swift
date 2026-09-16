// OmabarClient.swift – IPC client for the Omabar daemon
// ─────────────────────────────────────────────────────────────────
// Wraps the length-prefixed JSON protocol over a Unix domain socket
// at /tmp/omabar_$USER.socket.
//
// The daemon replies to every message and may push unsolicited events
// on the same socket.  This client reads continuously on a dedicated
// thread: incoming "event" messages are dispatched to the event
// handler; reply messages resolve the single outstanding request.
//
// Usage:
//   let client = OmabarClient()
//   try client.connect()
//   client.eventHandler = { event in await plugin.onEvent(event) }
//   try client.subscribe(to: OmabarEvent.BuiltinEvent.builtinNames)
//   try client.update(item: "battery", label: "87%")

import Foundation

enum OmabarMessage {
    static func register(name: String) -> String {
        "{\"type\":\"register\",\"name\":\(name.jsonQuoted)}"
    }

    static func subscribe(wireNames: [String]) -> String {
        let names = wireNames.map { $0.jsonQuoted }.joined(separator: ",")
        return "{\"type\":\"subscribe\",\"events\":[\(names)]}"
    }

    static func update(item: String,
                       icon: String? = nil,
                       label: String? = nil,
                       backgroundColor: String? = nil) -> String {
        var fields = "\"item\":\(item.jsonQuoted)"
        if let icon = icon { fields += ",\"icon\":\(icon.jsonQuoted)" }
        if let label = label { fields += ",\"label\":\(label.jsonQuoted)" }
        if let bg = backgroundColor { fields += ",\"background_color\":\(bg.jsonQuoted)" }
        return "{\"type\":\"update\",\(fields)}"
    }

    static func query(item: String) -> String {
        "{\"type\":\"query\",\"item\":\(item.jsonQuoted)}"
    }

    static func trigger(event: String, data: String? = nil) -> String {
        var fields = "\"event\":\(event.jsonQuoted)"
        if let data = data { fields += ",\"data\":\(data)" }
        return "{\"type\":\"trigger\",\(fields)}"
    }
}

// MARK: – OmabarClient

/// Manages a connection to the omabar daemon's Unix socket.
///
/// All socket I/O is serialised on a dedicated reader thread.  Set
/// `eventHandler` to receive daemon-pushed events (e.g. DND updates).
/// Request/response calls (subscribe, update, query, trigger, …) are
/// async/await and resolve when the matching reply arrives; they may
/// be used concurrently with event delivery.

public final class OmabarClient: @unchecked Sendable {

    public init() {
        let user = ProcessInfo.processInfo.environment["USER"] ?? "unknown"
        self.socketPath = "/tmp/omabar_\(user).socket"
    }

    /// Initializer with an explicit socket path (primarily for tests).
    public init(socketPath: String) {
        self.socketPath = socketPath
    }

    private let socketPath: String

    // ── State (guarded by lock) ──────────────────────────────────

    private let lock = NSLock()
    private var fd: Int32 = -1
    private var connected = false
    private var reader: Thread?
    private var stopReader = false
    /// The single outstanding request's continuation, or nil when idle.
    private var pendingReply: CheckedContinuation<[String: Any], Error>?
    private var pendingReplyQueue: DispatchQueue?

    /// Called (on a background queue) for every daemon-pushed event.
    /// Setting this enables the reader loop; leave nil to ignore events.
    public var eventHandler: (@Sendable (OmabarEvent) async -> Void)?

    public var isConnected: Bool {
        lock.lock(); defer { lock.unlock() }
        return connected
    }

    // ── Lifecycle ───────────────────────────────────────────────

    /// Open the Unix socket to the daemon.  Throws on failure.
    public func connect() throws -> Void {
        lock.lock(); defer { lock.unlock() }
        guard !connected else { return }

        let sock = socket(AF_UNIX, SOCK_STREAM, 0)
        guard sock >= 0 else { throw OmabarClientError.socketCreationFailed }

        var addr = sockaddr_un()
        memset(&addr, 0, MemoryLayout<sockaddr_un>.size)
        addr.sun_family = sa_family_t(AF_UNIX)
        _ = socketPath.withCString { path in
            withUnsafeMutableBytes(of: &addr.sun_path) { raw in
                memcpy(raw.baseAddress!, path, min(strlen(path), raw.count - 1))
            }
        }
        let len = MemoryLayout<sockaddr_un>.size
        let result = withUnsafePointer(to: &addr) { ptr in
            ptr.withMemoryRebound(to: sockaddr.self, capacity: 1) { sa in
                Darwin.connect(sock, sa, socklen_t(len))
            }
        }
        guard result == 0 else {
            Darwin.close(sock)
            throw OmabarClientError.connectFailed
        }

        fd = sock
        connected = true
        stopReader = false

        // Start the dedicated reader thread.
        reader = Thread { [weak self] in self?.readerLoop() }
        reader?.start()
    }

    /// Close the connection and stop the reader thread.
    public func disconnect() {
        lock.lock()
        defer { lock.unlock() }
        guard connected else { return }
        stopReader = true
        Darwin.close(fd)
        fd = -1
        connected = false
        let pending = pendingReply
        pendingReply = nil
        pending?.resume(throwing: OmabarClientError.notConnected)

        if let r = reader { r.cancel() }
        reader = nil
    }

    deinit { disconnect() }

    // ── Request / response (async) ──────────────────────────────

    /// Send a message and wait for the daemon's reply.
    /// Only one request may be in flight at a time; a second concurrent
    /// `request` call throws `.requestInFlight`.
    public func request(_ message: String) async throws -> [String: Any] {
        try await withCheckedThrowingContinuation { continuation in
            lock.lock()
            guard connected else {
                lock.unlock()
                continuation.resume(throwing: OmabarClientError.notConnected)
                return
            }
            guard pendingReply == nil else {
                lock.unlock()
                continuation.resume(throwing: OmabarClientError.requestInFlight)
                return
            }
            pendingReply = continuation
            pendingReplyQueue = DispatchQueue.global()
            let ok = sendRawLocked(message)
            guard ok else {
                pendingReply = nil
                lock.unlock()
                continuation.resume(throwing: OmabarClientError.writeFailed)
                return
            }
            lock.unlock()
        }
    }

    /// Fire-and-forget send: sends and discards the daemon's reply.
    @discardableResult
    public func send(_ message: String) async throws -> Bool {
        let reply = try await request(message)
        return reply["ok"] as? Bool ?? false
    }

    // ── Convenience helpers ─────────────────────────────────────

    /// Subscribe to the given builtin event wire names.
    @discardableResult
    public func subscribe(to wireNames: [String]) async throws -> Bool {
        let reply = try await request(OmabarMessage.subscribe(wireNames: wireNames))
        return reply["ok"] as? Bool ?? false
    }

    /// Update an item's visible state.
    @discardableResult
    public func update(item: String,
                       icon: String? = nil,
                       label: String? = nil,
                       backgroundColor: String? = nil) async throws -> Bool {
        let msg = OmabarMessage.update(item: item,
                                       icon: icon,
                                       label: label,
                                       backgroundColor: backgroundColor)
        let reply = try await request(msg)
        return reply["ok"] as? Bool ?? false
    }

    /// Query the current state of a bar item; nil if item does not exist.
    public func query(item: String) async throws -> OmabarItemState? {
        let reply = try await request(OmabarMessage.query(item: item))
        guard reply["ok"] as? Bool == true,
              let name = reply["item"] as? String else { return nil }
        return OmabarItemState(
            item: name,
            icon: reply["icon"] as? String ?? "",
            label: reply["label"] as? String ?? "",
            background_color: reply["background_color"] as? String ?? "0x00000000"
        )
    }

    /// Trigger a custom event by name with optional JSON data.
    @discardableResult
    public func trigger(event: String, data: String? = nil) async throws -> Bool {
        let reply = try await request(OmabarMessage.trigger(event: event, data: data))
        return reply["ok"] as? Bool ?? false
    }

    // ── Packet construction / serialisation helper ──────────────

    private func sendRawLocked(_ json: String) -> Bool {
        let data = Array(json.utf8)
        let len = UInt32(data.count).bigEndian
        var header = len
        let headerBytes = withUnsafeBytes(of: &header) { Array($0) }
        let hs = headerBytes.withUnsafeBufferPointer { Darwin.write(fd, $0.baseAddress!, $0.count) }
        guard hs == MemoryLayout<UInt32>.size else { return false }
        let ps = data.withUnsafeBufferPointer { Darwin.write(fd, $0.baseAddress!, $0.count) }
        return ps == data.count
    }

    // ── Reader loop (dedicated thread) ──────────────────────────

    private func readerLoop() {
        while true {
            guard !stopReader, fd >= 0 else { break }

            let headerBytes = readExact(fd, 4)
            guard headerBytes.count == 4 else { break }

            let len = headerBytes.withUnsafeBytes { $0.load(fromByteOffset: 0, as: UInt32.self).bigEndian }
            guard len > 0, len <= 16 * 1024 * 1024 else { break }

            let payloadBytes = readExact(fd, Int(len))
            guard payloadBytes.count == Int(len) else { break }

            guard let str = String(bytes: payloadBytes, encoding: .utf8),
                  let data = str.data(using: .utf8),
                  let obj = try? JSONSerialization.jsonObject(with: data) as? [String: Any]
            else { break }

            let type = obj["type"] as? String
            if type == "event", let name = obj["name"] as? String {
                let event = OmabarEvent(wireName: name, data: dataString(obj["data"]))
                if let handler = eventHandler {
                    Task { await handler(event) }
                }
                continue
            }

            // Reply to an outstanding request.
            lock.lock()
            let continuation = pendingReply
            pendingReply = nil
            let queue = pendingReplyQueue
            pendingReplyQueue = nil
            lock.unlock()
            if let continuation = continuation {
                let q = queue ?? DispatchQueue.global()
                q.async { continuation.resume(returning: obj) }
            }
        }

        // Connection torn down: resolve any pending request.
        lock.lock()
        let pending = pendingReply
        pendingReply = nil
        let q = pendingReplyQueue
        pendingReplyQueue = nil
        lock.unlock()
        pending?.resume(throwing: OmabarClientError.notConnected)
        _ = q
    }

    private func readExact(_ fd: Int32, _ count: Int) -> [UInt8] {
        var buf = [UInt8](repeating: 0, count: count)
        var done = 0
        while done < count {
            let n = buf[done...].withUnsafeMutableBufferPointer { ptr in
                Darwin.read(fd, ptr.baseAddress!, count - done)
            }
            if n <= 0 { return [] }
            done += n
        }
        return buf
    }

    /// JSONSerialization parses `"data"` into a dict/array/string —
    /// keep it as the raw JSON text for the plugin.
    private func dataString(_ value: Any?) -> String? {
        guard let value = value else { return nil }
        if value is NSNull { return nil }
        if let s = value as? String { return s }
        let options: JSONSerialization.WritingOptions = [.fragmentsAllowed]
        guard let data = try? JSONSerialization.data(withJSONObject: value, options: options) else {
            return String(describing: value)
        }
        return String(data: data, encoding: .utf8)
    }
}

// MARK: – Errors

public enum OmabarClientError: Error, LocalizedError {
    case notConnected
    case socketCreationFailed
    case connectFailed
    case writeFailed
    case readFailed
    case invalidMessageLength
    case invalidJSON
    case requestInFlight

    public var errorDescription: String? {
        switch self {
        case .notConnected:         return "Not connected to daemon"
        case .socketCreationFailed: return "Failed to create Unix socket"
        case .connectFailed:        return "Failed to connect to daemon socket"
        case .writeFailed:          return "Write to daemon failed"
        case .readFailed:           return "Read from daemon failed"
        case .invalidMessageLength: return "Invalid message length from daemon"
        case .invalidJSON:          return "Invalid JSON from daemon"
        case .requestInFlight:      return "Another request is already in flight"
        }
    }
}

// MARK: – Internal helpers

extension String {
    /// JSON-quoted string value (escapes backslash and double-quote).
    var jsonQuoted: String {
        let escaped = self
            .replacingOccurrences(of: "\\", with: "\\\\")
            .replacingOccurrences(of: "\"", with: "\\\"")
        return "\"\(escaped)\""
    }
}