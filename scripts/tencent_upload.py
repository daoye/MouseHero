#!/usr/bin/env python3
"""Upload a file or directory to Tencent Cloud COS.

Requirements:
  pip install cos-python-sdk-v5

Credentials can be supplied via environment variables or CLI arguments:
  COS_SECRET_ID
  COS_SECRET_KEY
  COS_BUCKET
  COS_REGION
"""

from __future__ import annotations

import argparse
import mimetypes
import os
import time
from pathlib import Path
from typing import Iterable, Tuple

from qcloud_cos import CosConfig, CosS3Client
from qcloud_cos.cos_exception import CosClientError, CosServiceError

_MAX_RETRIES = 10
_RETRY_DELAY_SECONDS = 2

def _ensure_directory(path: Path) -> None:
    if not path.exists():
        raise FileNotFoundError(f"Path does not exist: {path}")


def _iter_files(source: Path) -> Iterable[Tuple[Path, str]]:
    if source.is_file():
        yield source, source.name
        return

    for path in source.rglob("*"):
        if path.is_file():
            relative_key = path.relative_to(source).as_posix()
            yield path, relative_key


def _build_client(secret_id: str, secret_key: str, region: str) -> CosS3Client:
    if not region:
        raise ValueError("Region is required for COS uploads")

    config = CosConfig(
        Region=region,
        SecretId=secret_id,
        SecretKey=secret_key,
    )
    return CosS3Client(config)


def _upload_file(client: CosS3Client, bucket: str, file_path: Path, key: str) -> None:
    last_error: Exception | None = None
    for attempt in range(1, _MAX_RETRIES + 1):
        try:
            client.upload_file(
                Bucket=bucket,
                Key=key,
                LocalFilePath=str(file_path)
            )
            return
        except CosServiceError as exc:
            last_error = RuntimeError(
                f"Failed to upload {key}: {exc.get_error_msg()} (code: {exc.get_error_code()}, request_id: {exc.get_request_id()})"
            )
        except CosClientError as exc:
            last_error = RuntimeError(f"Failed to upload {key}: {exc}")

        if attempt < _MAX_RETRIES:
            print(f"Upload failed for {key} (attempt {attempt}/{_MAX_RETRIES}), retrying in {_RETRY_DELAY_SECONDS}s")
            time.sleep(_RETRY_DELAY_SECONDS)
        else:
            assert last_error is not None
            raise last_error


def deploy(
    source: Path,
    secret_id: str,
    secret_key: str,
    bucket: str,
    region: str,
    remote_prefix: str,
) -> None:
    _ensure_directory(source)
    client = _build_client(secret_id, secret_key, region)
    prefix = remote_prefix
    if prefix and not prefix.endswith("/"):
        prefix += "/"

    files = list(_iter_files(source))
    if not files:
        print(f"No files found under {source}, skip uploading")
        return

    print(f"Uploading {len(files)} files to bucket '{bucket}' in region '{region}'")

    for index, (path, relative_key) in enumerate(files, start=1):
        remote_key = f"{prefix}{relative_key}" if prefix else relative_key
        _upload_file(client, bucket, path, remote_key)
        print(f"[{index}/{len(files)}] Uploaded {relative_key} -> {remote_key}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Upload files or directories to Tencent Cloud COS")
    parser.add_argument("--source", default="dist", help="Path to the file or directory to upload")
    parser.add_argument("--secret-id", default=os.getenv("COS_SECRET_ID"), help="COS SecretId")
    parser.add_argument("--secret-key", default=os.getenv("COS_SECRET_KEY"), help="COS SecretKey")
    parser.add_argument("--bucket", default=os.getenv("COS_BUCKET"), help="Target bucket name (e.g. example-1250000000)")
    parser.add_argument("--region", default=os.getenv("COS_REGION"), help="COS region, e.g. ap-shanghai")
    parser.add_argument("--remote-prefix", default="", help="Optional remote prefix inside the bucket")

    args = parser.parse_args()

    missing = [
        name
        for name, value in (
            ("secret id", args.secret_id),
            ("secret key", args.secret_key),
            ("bucket", args.bucket),
            ("region", args.region),
        )
        if not value
    ]
    if missing:
        parser.error("Missing parameters: " + ", ".join(missing))

    try:
        deploy(
            Path(args.source).resolve(),
            args.secret_id,
            args.secret_key,
            args.bucket,
            args.region,
            args.remote_prefix,
        )
    except Exception as exc:
        raise SystemExit(str(exc)) from exc


if __name__ == "__main__":
    main()
