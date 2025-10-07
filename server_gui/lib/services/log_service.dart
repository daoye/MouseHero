import 'package:flutter/foundation.dart';
import 'package:server_gui/services/server_process_service.dart';

class LogService {
  final ServerProcessService _serverProcessService = ServerProcessService();

  /// 获取日志内容
  Future<String> getLogs() async {
    try {
      // 从服务进程获取日志
      final logs = _serverProcessService.getLogs();

      // 将日志列表转换为字符串
      return logs.join('\n');
    } catch (e) {
      debugPrint('获取日志失败: $e');
      return '获取日志失败: ${e.toString()}';
    }
  }

  /// 清空日志
  Future<void> clearLogs() async {
    try {
      // 清空服务进程中的日志缓冲区
      _serverProcessService.clearLogs();
    } catch (e) {
      debugPrint('清空日志失败: $e');
      rethrow;
    }
  }
}
