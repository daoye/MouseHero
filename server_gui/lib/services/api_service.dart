import 'package:fluent_ui/fluent_ui.dart';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:msgpack_dart/msgpack_dart.dart' as msgpack;

class ApiService {
  Future<Map<String, dynamic>?> getStatus({
    String? origin = '127.0.0.1:21218',
  }) async {
    final response = await http.get(Uri.parse('http://$origin/status'));

    if (response.statusCode == 200) {
      final data = msgpack.deserialize(response.bodyBytes);
      if (data is Map) {
        final Map<String, dynamic> map =
            data.map((key, value) => MapEntry(key.toString(), value));

        final rawClients = map['connected_clients'];
        if (rawClients is List) {
          final List<Map<String, dynamic>> clients = rawClients.map((e) {
            if (e is Map) {
              final m = e.map((k, v) => MapEntry(k.toString(), v));
              return {
                'uid': m['uid'],
                'ip': m['address'] ?? m['ip'],
                'connected_at': m['connected_at'],
              };
            }
            return <String, dynamic>{};
          }).toList();
          map['clients'] = clients;
        } else {
          map['clients'] = <Map<String, dynamic>>[];
        }

        return map;
      }
    } else {
      debugPrint('Failed to get status: ${response.statusCode}');
    }

    return null;
  }
}
