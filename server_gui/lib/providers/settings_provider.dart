import 'package:flutter/material.dart';
import 'package:server_gui/services/settings_service.dart';

/// 统一的设置提供者，用于管理所有GUI设置
/// 整合了主题设置、语言设置、窗口设置和服务端状态记忆
class SettingsProvider with ChangeNotifier {
  final SettingsService _settingsService = SettingsService();

  // 窗口设置
  bool _hideOnAutoStart = false;
  bool _serverLastRunning = false;

  // 主题设置
  ThemeMode _themeMode = ThemeMode.system;

  // 语言设置
  Locale? _locale;

  // 日志页面设置（不持久化）
  bool _logAutoRefresh = true;

  // 获取设置值
  bool get hideOnAutoStart => _hideOnAutoStart;
  bool get serverLastRunning => _serverLastRunning;
  ThemeMode get themeMode => _themeMode;
  Locale? get locale => _locale;
  bool get logAutoRefresh => _logAutoRefresh;

  SettingsProvider() {
    _loadSettings();
  }

  /// 加载所有设置
  Future<void> _loadSettings() async {
    try {
      await _settingsService.init();

      try {
        // 加载窗口设置
        _hideOnAutoStart = await _settingsService.getHideOnAutoStart();
        _serverLastRunning = await _settingsService.getServerLastRunning();
      } catch (e) {
        debugPrint('加载窗口设置失败: $e');
        // 使用默认值
      }

      try {
        // 加载主题设置
        final themeModeValue = await _settingsService.getThemeMode();
        String themeModeString;
        
        // 处理可能的类型不匹配问题
        if (themeModeValue is int) {
          // 如果存储的是整数，转换为对应的字符串
          final intValue = themeModeValue;
          if (intValue == 0) {
            themeModeString = 'system';
          } else if (intValue == 1) {
            themeModeString = 'light';
          } else if (intValue == 2) {
            themeModeString = 'dark';
          } else {
            themeModeString = 'system';
          }
          // 将正确的字符串值写回设置
          await _settingsService.setThemeMode(themeModeString);
        } else {
          themeModeString = themeModeValue.toString();
        }
        
        switch (themeModeString) {
          case 'system':
            _themeMode = ThemeMode.system;
            break;
          case 'light':
            _themeMode = ThemeMode.light;
            break;
          case 'dark':
            _themeMode = ThemeMode.dark;
            break;
          default:
            _themeMode = ThemeMode.system;
        }
      } catch (e) {
        debugPrint('加载主题设置失败: $e');
        // 使用默认值
      }

      try {
        // 加载语言设置
        final localeValue = await _settingsService.getLocale();
        String localeString;
        
        // 处理可能的类型不匹配问题
        if (localeValue is int) {
          // 如果存储的是整数，转换为对应的字符串
          final intValue = localeValue;
          if (intValue == 0) {
            localeString = 'system';
          } else if (intValue == 1) {
            localeString = 'zh';
          } else if (intValue == 2) {
            localeString = 'en';
          } else {
            localeString = 'system';
          }
          // 将正确的字符串值写回设置
          await _settingsService.setLocale(localeString);
        } else {
          localeString = localeValue.toString();
        }
        
        if (localeString != 'system' && localeString.isNotEmpty) {
          _locale = Locale(localeString);
        }
      } catch (e) {
        debugPrint('加载语言设置失败: $e');
        // 使用默认值
      }

      notifyListeners();
    } catch (e) {
      debugPrint('加载设置失败: $e');
      // 使用默认值
    }
  }

  /// 设置开机自启时隐藏主界面
  Future<void> setHideOnAutoStart(bool value) async {
    if (_hideOnAutoStart == value) return;

    try {
      await _settingsService.setHideOnAutoStart(value);
      _hideOnAutoStart = value;
      notifyListeners();
    } catch (e) {
      debugPrint('设置开机自启时隐藏主界面失败: $e');
    }
  }

  /// 设置服务端运行状态
  Future<void> setServerLastRunning(bool value) async {
    if (_serverLastRunning == value) return;

    try {
      await _settingsService.setServerLastRunning(value);
      _serverLastRunning = value;
      notifyListeners();
    } catch (e) {
      debugPrint('设置服务端运行状态失败: $e');
    }
  }

  /// 设置主题模式
  Future<void> setThemeMode(ThemeMode mode) async {
    if (_themeMode == mode) return;

    try {
      String themeModeString;

      switch (mode) {
        case ThemeMode.system:
          themeModeString = 'system';
          break;
        case ThemeMode.light:
          themeModeString = 'light';
          break;
        case ThemeMode.dark:
          themeModeString = 'dark';
          break;
      }

      await _settingsService.setThemeMode(themeModeString);
      _themeMode = mode;
      notifyListeners();
    } catch (e) {
      debugPrint('设置主题模式失败: $e');
    }
  }

  /// 设置语言
  Future<void> setLocale(Locale newLocale) async {
    if (_locale == newLocale) return;

    try {
      await _settingsService.setLocale(newLocale.languageCode);
      _locale = newLocale;
      notifyListeners();
    } catch (e) {
      debugPrint('设置语言失败: $e');
    }
  }

  /// 重置为系统语言
  Future<void> resetLocale() async {
    if (_locale == null) return;

    try {
      await _settingsService.setLocale('system');
      _locale = null;
      notifyListeners();
    } catch (e) {
      debugPrint('重置语言设置失败: $e');
    }
  }

  /// 设置日志自动刷新状态（不持久化）
  void setLogAutoRefresh(bool value) {
    if (_logAutoRefresh == value) return;
    
    _logAutoRefresh = value;
    notifyListeners();
  }
}
