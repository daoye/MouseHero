#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import json
import sys
from tencentcloud.common import credential
from tencentcloud.common.profile.client_profile import ClientProfile
from tencentcloud.common.profile.http_profile import HttpProfile
from tencentcloud.common.exception.tencent_cloud_sdk_exception import TencentCloudSDKException
from tencentcloud.cdn.v20180606 import cdn_client, models


def purge_cdn_urls(urls):
    """Purge CDN cache for specific URLs"""
    try:
        # Read credentials from environment variables
        secret_id = os.getenv("TENCENTCLOUD_SECRET_ID")
        secret_key = os.getenv("TENCENTCLOUD_SECRET_KEY")
        
        if not secret_id or not secret_key:
            print("Error: TENCENTCLOUD_SECRET_ID and TENCENTCLOUD_SECRET_KEY must be set", file=sys.stderr)
            sys.exit(1)
        
        cred = credential.Credential(secret_id, secret_key)
        
        # Configure HTTP profile
        httpProfile = HttpProfile()
        httpProfile.endpoint = "cdn.tencentcloudapi.com"
        
        # Configure client profile
        clientProfile = ClientProfile()
        clientProfile.httpProfile = httpProfile
        
        # Create CDN client
        client = cdn_client.CdnClient(cred, "ap-guangzhou", clientProfile)
        
        # Create request
        req = models.PurgeUrlsCacheRequest()
        params = {
            "Urls": urls,
            "UrlEncode": False
        }
        req.from_json_string(json.dumps(params))
        
        # Execute request
        resp = client.PurgeUrlsCache(req)
        
        # Output response
        print("CDN URL purge successful:")
        print(resp.to_json_string())
        
    except TencentCloudSDKException as err:
        print(f"CDN purge failed: {err}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    # URLs to purge for MouseHero downloads
    urls_to_purge = [
        "https://cn-downloads-mousehero.aprilzz.com/downloads/MouseHero-Windows-x86_64-latest.zip",
        "https://cn-downloads-mousehero.aprilzz.com/downloads/MouseHero-Linux-x86_64-latest.AppImage",
        "https://cn-downloads-mousehero.aprilzz.com/downloads/MouseHero-macOS-arm64-latest.dmg",
        "https://cn-downloads-mousehero.aprilzz.com/downloads/MouseHero-macOS-x86_64-latest.dmg"
    ]
    
    print(f"Purging {len(urls_to_purge)} URLs from CDN...")
    purge_cdn_urls(urls_to_purge)
