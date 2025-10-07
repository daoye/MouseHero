import Cocoa
import FlutterMacOS

@main
class AppDelegate: FlutterAppDelegate {
  override func applicationWillTerminate(_ aNotification: Notification) {
    _killChildProcess()
  }

  private func _killChildProcess() {
    let fileManager = FileManager.default
    let appSupportURLs = fileManager.urls(for: .libraryDirectory, in: .userDomainMask)
    guard let libDir = appSupportURLs.first else { return }
    let appDir = libDir.appendingPathComponent("MouseHero", isDirectory: true)
    let pidFile = appDir.appendingPathComponent("server.pid")

    do {
      let pidStr = try String(contentsOf: pidFile, encoding: .utf8)
      if let pid = Int32(pidStr.trimmingCharacters(in: .whitespacesAndNewlines)) {
        kill(pid, SIGTERM)
      }
    } catch {
    }
  }

  override func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return false
  }

  override func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
    return true
  }
  
  override func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
    if !flag {
      NSApp.windows.first?.makeKeyAndOrderFront(self)
    }
    return true
  }
}
