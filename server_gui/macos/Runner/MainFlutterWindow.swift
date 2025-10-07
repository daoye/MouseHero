import Cocoa
import FlutterMacOS
import window_manager
#if canImport(LaunchAtLogin)
// Add the LaunchAtLogin module required by launch_at_startup plugin on macOS
import LaunchAtLogin
#endif

class MainFlutterWindow: NSWindow {
  override func awakeFromNib() {
    let flutterViewController = FlutterViewController()
    let windowFrame = self.frame
    self.contentViewController = flutterViewController
    self.setFrame(windowFrame, display: true)

    #if canImport(LaunchAtLogin)
    FlutterMethodChannel(
      name: "launch_at_startup",
      binaryMessenger: flutterViewController.engine.binaryMessenger
    ).setMethodCallHandler { (call: FlutterMethodCall, result: @escaping FlutterResult) in
      switch call.method {
      case "launchAtStartupIsEnabled":
        result(LaunchAtLogin.isEnabled)
      case "launchAtStartupSetEnabled":
        if let arguments = call.arguments as? [String: Any],
           let setEnabledValue = arguments["setEnabledValue"] as? Bool {
          LaunchAtLogin.isEnabled = setEnabledValue
        }
        result(nil)
      default:
        result(FlutterMethodNotImplemented)
      }
    }
    #else
    // Package not linked yet; method channel intentionally omitted to avoid build failure.
    // Dart side guards MissingPluginException.
    #endif
    
    RegisterGeneratedPlugins(registry: flutterViewController)

    super.awakeFromNib()
  }

  override public func order(_ place: NSWindow.OrderingMode, relativeTo otherWin: Int) {
    super.order(place, relativeTo: otherWin)
    hiddenWindowAtLaunch()
  }
}
