import 'dart:convert';

import 'package:http/http.dart' as http;

class UpdateCheckResult {
  const UpdateCheckResult({
    required this.hasUpdate,
    this.latestVersion,
    this.latestBuildNumber,
    this.releaseUrl,
    this.errorMessage,
  });

  final bool hasUpdate;
  final String? latestVersion;
  final String? latestBuildNumber;
  final Uri? releaseUrl;
  final String? errorMessage;

  bool get hasError => errorMessage != null;

  String? get versionLabel {
    if (latestVersion == null) {
      return null;
    }
    if (latestBuildNumber != null && latestBuildNumber!.isNotEmpty) {
      return '${latestVersion!}+${latestBuildNumber!}';
    }
    return latestVersion;
  }
}

class UpdateService {
  UpdateService({http.Client? httpClient})
    : _client = httpClient ?? http.Client();

  static final Uri defaultReleaseUri =
      Uri.parse('https://mousehero.aprilzz.com');
  static final Uri _defaultEndpoint =
      Uri.parse('https://mousehero.aprilzz.com/version.json');

  final http.Client _client;

  Future<UpdateCheckResult> checkForUpdates({
    required String currentVersion,
    String? currentBuildNumber,
  }) async {
    try {
      final response = await _client.get(_defaultEndpoint);
      if (response.statusCode != 200) {
        return UpdateCheckResult(
          hasUpdate: false,
          errorMessage: 'HttpError: ${response.statusCode}',
        );
      }

      final raw = jsonDecode(response.body);
      if (raw is! Map<String, dynamic>) {
        return const UpdateCheckResult(
          hasUpdate: false,
          errorMessage: 'InvalidPayload',
        );
      }

      final String? remoteVersion = raw['version']?.toString();
      final String? remoteBuild = raw['build']?.toString();
      final Uri releaseUri = Uri.tryParse(raw['url']?.toString() ?? '') ??
          defaultReleaseUri;

      if (remoteVersion == null || remoteVersion.isEmpty) {
        return const UpdateCheckResult(
          hasUpdate: false,
          errorMessage: 'MissingVersion',
        );
      }

      final bool hasUpdate = _isRemoteNewer(
        remoteVersion,
        remoteBuild,
        currentVersion,
        currentBuildNumber,
      );

      return UpdateCheckResult(
        hasUpdate: hasUpdate,
        latestVersion: remoteVersion,
        latestBuildNumber: remoteBuild,
        releaseUrl: releaseUri,
      );
    } catch (error) {
      return UpdateCheckResult(
        hasUpdate: false,
        errorMessage: error.toString(),
      );
    }
  }

  bool _isRemoteNewer(
    String remoteVersion,
    String? remoteBuild,
    String currentVersion,
    String? currentBuild,
  ) {
    final int versionCompare =
        _compareVersionStrings(remoteVersion, currentVersion);
    if (versionCompare > 0) {
      return true;
    }
    if (versionCompare < 0) {
      return false;
    }

    final int? remoteBuildNumber = int.tryParse(remoteBuild ?? '');
    final int? currentBuildNumber = int.tryParse(currentBuild ?? '');

    if (remoteBuildNumber != null && currentBuildNumber != null) {
      return remoteBuildNumber > currentBuildNumber;
    }
    if (remoteBuildNumber != null && currentBuildNumber == null) {
      return true;
    }
    return false;
  }

  int _compareVersionStrings(String a, String b) {
    final List<int> partsA = _normalizeVersion(a);
    final List<int> partsB = _normalizeVersion(b);
    final int maxLength = partsA.length > partsB.length ? partsA.length : partsB.length;

    for (int i = 0; i < maxLength; i++) {
      final int va = i < partsA.length ? partsA[i] : 0;
      final int vb = i < partsB.length ? partsB[i] : 0;
      if (va > vb) {
        return 1;
      }
      if (va < vb) {
        return -1;
      }
    }
    return 0;
  }

  List<int> _normalizeVersion(String value) {
    final sanitized = value.split('+').first;
    return sanitized
        .split('.')
        .map((segment) => int.tryParse(segment) ?? 0)
        .toList();
  }
}
