// ignore_for_file: unused_element
 
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// 统一的GUI设置服务类，仅用于管理server_gui的界面和行为相关设置
/// 注意：该服务仅管理GUI层面的设置，不包含核心server的配置
/// server的配置由ConfigService管理，两者职责完全不同
class SettingsService {
  static final SettingsService _instance = SettingsService._internal();
  
  factory SettingsService() {
    return _instance;
  }
  
  SettingsService._internal();
  
  bool _initialized = false;
  late SharedPreferences _prefs;
  
  // GUI设置键名常量
  static const String keyHideOnAutoStart = 'hide_on_auto_start'; // 开机自启时隐藏主界面
  static const String keyServerLastRunning = 'server_last_running'; // 服务端上次运行状态（仅用于GUI层面记忆）
  static const String keyThemeMode = 'theme_mode';               // GUI主题模式
  static const String keyLocale = 'locale';                     // GUI语言设置
  
  /// 初始化设置服务
  Future<void> init() async {
    if (_initialized) return;
    
    try {
      _prefs = await SharedPreferences.getInstance();
      _initialized = true;
    } catch (e) {
      debugPrint('初始化设置服务失败: $e');
      _initialized = true; // 防止重复尝试初始化
    }
  }
  
  // SharedPreferences 会自动保存，不需要额外的保存方法
  
  /// 确保服务已初始化
  Future<void> _ensureInitialized() async {
    if (!_initialized) {
      await init();
    }
  }
  
  /// 获取布尔类型设置（内部方法）
  Future<bool> _getBool(String key, {bool defaultValue = false}) async {
    await _ensureInitialized();
    return _prefs.getBool(key) ?? defaultValue;
  }
  
  /// 设置布尔类型设置（内部方法）
  Future<void> _setBool(String key, bool value) async {
    await _ensureInitialized();
    await _prefs.setBool(key, value);
  }
  
  /// 获取字符串类型设置（内部方法）
  Future<String> _getString(String key, {String defaultValue = ''}) async {
    await _ensureInitialized();
    return _prefs.getString(key) ?? defaultValue;
  }
  
  /// 设置字符串类型设置（内部方法）
  Future<void> _setString(String key, String value) async {
    await _ensureInitialized();
    await _prefs.setString(key, value);
  }
  
  /// 获取整数类型设置（内部方法）
  Future<int> _getInt(String key, {int defaultValue = 0}) async {
    await _ensureInitialized();
    return _prefs.getInt(key) ?? defaultValue;
  }
  
  /// 设置整数类型设置（内部方法）
  Future<void> _setInt(String key, int value) async {
    await _ensureInitialized();
    await _prefs.setInt(key, value);
  }
  
  /// 获取双精度浮点数类型设置（内部方法）
  Future<double> _getDouble(String key, {double defaultValue = 0.0}) async {
    await _ensureInitialized();
    return _prefs.getDouble(key) ?? defaultValue;
  }
  
  /// 设置双精度浮点数类型设置（内部方法）
  Future<void> _setDouble(String key, double value) async {
    await _ensureInitialized();
    await _prefs.setDouble(key, value);
  }
  
  /// 检查设置是否存在（内部方法）
  Future<bool> _containsKey(String key) async {
    await _ensureInitialized();
    return _prefs.containsKey(key);
  }
  
  /// 删除设置（内部方法）
  Future<void> _remove(String key) async {
    await _ensureInitialized();
    await _prefs.remove(key);
  }
  
  /// 清除所有设置（内部方法）
  Future<void> _clear() async {
    await _ensureInitialized();
    await _prefs.clear();
  }
  
  // 以下是特定设置的便捷方法
  

  
  /// 获取开机自启时隐藏主界面设置
  Future<bool> getHideOnAutoStart() async {
    return _getBool(keyHideOnAutoStart, defaultValue: false);
  }
  
  /// 设置开机自启时隐藏主界面
  Future<void> setHideOnAutoStart(bool value) async {
    await _setBool(keyHideOnAutoStart, value);
  }
  
  /// 获取服务端上次运行状态
  Future<bool> getServerLastRunning() async {
    return _getBool(keyServerLastRunning, defaultValue: false);
  }
  
  /// 设置服务端运行状态
  Future<void> setServerLastRunning(bool value) async {
    await _setBool(keyServerLastRunning, value);
  }
  
  /// 获取主题模式
  Future<dynamic> getThemeMode() async {
    await _ensureInitialized();
    
    try {
      // 先尝试获取字符串值
      if (_prefs.containsKey(keyThemeMode)) {
        if (_prefs.get(keyThemeMode) is String) {
          return _prefs.getString(keyThemeMode) ?? 'system';
        } else if (_prefs.get(keyThemeMode) is int) {
          // 如果存储的是整数，直接返回整数值，由Provider处理转换
          return _prefs.getInt(keyThemeMode);
        }
      }
      return 'system';
    } catch (e) {
      debugPrint('获取主题模式失败: $e');
      return 'system';
    }
  }
  
  /// 设置主题模式
  Future<void> setThemeMode(String value) async {
    await _setString(keyThemeMode, value);
  }
  
  /// 获取语言设置
  Future<dynamic> getLocale() async {
    await _ensureInitialized();
    
    try {
      // 先尝试获取字符串值
      if (_prefs.containsKey(keyLocale)) {
        if (_prefs.get(keyLocale) is String) {
          return _prefs.getString(keyLocale) ?? 'system';
        } else if (_prefs.get(keyLocale) is int) {
          // 如果存储的是整数，直接返回整数值，由Provider处理转换
          return _prefs.getInt(keyLocale);
        }
      }
      return 'system';
    } catch (e) {
      debugPrint('获取语言设置失败: $e');
      return 'system';
    }
  }
  
  /// 设置语言
  Future<void> setLocale(String value) async {
    await _setString(keyLocale, value);
  }
}
