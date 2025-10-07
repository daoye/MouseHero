import 'dart:io';

class NetworkService {
  /// 单例实例
  static final NetworkService _instance = NetworkService._internal();

  factory NetworkService() {
    return _instance;
  }

  NetworkService._internal();

  /// 获取当前设备的局域网IP地址列表
  ///
  /// 返回一个包含所有非回环IPv4地址的列表
  /// 如果无法获取IP地址，则返回空列表
  Future<List<String>> getIpAddresses() async {
    try {
      final List<String> result = [];
      final interfaces = await NetworkInterface.list(
        includeLoopback: false,
        type: InternetAddressType.IPv4,
      );

      for (var interface in interfaces) {
        for (var addr in interface.addresses) {
          if (addr.type == InternetAddressType.IPv4) {
            result.add(addr.address);
          }
        }
      }

      return result;
    } catch (_) {
      return [];
    }
  }

  /// 获取首选局域网IP地址
  ///
  /// 返回第一个找到的非回环IPv4地址
  /// 如果没有找到合适的地址，则返回空字符串
  Future<String> getPreferredIp() async {
    final ipList = await getIpAddresses();

    String value = '127.0.0.1';

    bool finded = false;
    for (var ip in ipList) {
      if (isLanIp(ip)) {
        value = ip;
        finded = true;
        break;
      }
    }

    if (!finded && ipList.isNotEmpty) {
      value = ipList.first;
    }

    return value;
  }

  /// 检查给定的IP地址是否为局域网IP
  ///
  /// 参数:
  ///   [ip] - 要检查的IP地址
  ///
  /// 返回:
  ///   如果是局域网IP则返回true，否则返回false
  bool isLanIp(String ip) {
    try {
      final addr = InternetAddress(ip);
      if (addr.type != InternetAddressType.IPv4) return false;

      // 检查是否为私有IP地址范围
      // 10.0.0.0/8
      // 172.16.0.0/12
      // 192.168.0.0/16
      final octets = ip.split('.').map(int.parse).toList();

      if (octets[0] == 10) return true;
      if (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31) return true;
      if (octets[0] == 192 && octets[1] == 168) return true;

      return false;
    } catch (e) {
      return false;
    }
  }
}
