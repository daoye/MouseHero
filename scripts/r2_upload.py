#!/usr/bin/env python3
"""Upload files or directories to Cloudflare R2 using the S3 compatible API.

Requirements:
  pip install boto3

Credentials can be supplied via environment variables or CLI arguments:
  CLOUDFLARE_R2_ACCESS_KEY_ID
  CLOUDFLARE_R2_SECRET_ACCESS_KEY
  CLOUDFLARE_R2_ACCOUNT_ID
  CLOUDFLARE_R2_BUCKET
"""

from __future__ import annotations

import argparse
import mimetypes
import os
from pathlib import Path
from typing import Iterable, Tuple

import boto3
from botocore.exceptions import BotoCoreError, ClientError


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


def _build_client(access_key: str, secret_key: str, account_id: str, endpoint: str | None) -> boto3.client:
    if endpoint:
        endpoint_url = endpoint.rstrip("/")
    else:
        endpoint_url = f"https://{account_id}.r2.cloudflarestorage.com"

    session = boto3.session.Session()
    return session.client(
        service_name="s3",
        aws_access_key_id=access_key,
        aws_secret_access_key=secret_key,
        endpoint_url=endpoint_url,
    )


def _upload_file(client: boto3.client, bucket: str, file_path: Path, key: str) -> None:
    mime_type, _ = mimetypes.guess_type(str(file_path))
    extra_args = {}
    if mime_type:
        extra_args["ContentType"] = mime_type

    upload_kwargs = {"ExtraArgs": extra_args} if extra_args else {}

    try:
        client.upload_file(str(file_path), bucket, key, **upload_kwargs)
    except (BotoCoreError, ClientError) as exc:
        raise RuntimeError(f"Failed to upload {key}: {exc}") from exc


def deploy(source: Path, access_key: str, secret_key: str, account_id: str, bucket: str, remote_prefix: str, endpoint: str | None) -> None:
    _ensure_directory(source)
    client = _build_client(access_key, secret_key, account_id, endpoint)

    prefix = remote_prefix
    if prefix and not prefix.endswith("/"):
        prefix += "/"

    files = list(_iter_files(source))
    if not files:
        print(f"No files found under {source}, skip uploading")
        return

    print(f"Uploading {len(files)} files to Cloudflare R2 bucket '{bucket}'")

    for index, (path, relative_key) in enumerate(files, start=1):
        remote_key = f"{prefix}{relative_key}" if prefix else relative_key
        _upload_file(client, bucket, path, remote_key)
        print(f"[{index}/{len(files)}] Uploaded {relative_key} -> {remote_key}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Upload files or directories to Cloudflare R2")
    parser.add_argument("--source", default="dist", help="Path to the file or directory to upload")
    parser.add_argument("--access-key", default=os.getenv("CLOUDFLARE_R2_ACCESS_KEY_ID"), help="Cloudflare R2 access key ID")
    parser.add_argument("--secret-key", default=os.getenv("CLOUDFLARE_R2_SECRET_ACCESS_KEY"), help="Cloudflare R2 secret access key")
    parser.add_argument("--account-id", default=os.getenv("CLOUDFLARE_R2_ACCOUNT_ID"), help="Cloudflare account ID")
    parser.add_argument("--bucket", default=os.getenv("CLOUDFLARE_R2_BUCKET"), help="Target bucket name")
    parser.add_argument("--remote-prefix", default="", help="Optional remote prefix inside the bucket")
    parser.add_argument("--endpoint", default=os.getenv("CLOUDFLARE_R2_ENDPOINT"), help="Custom endpoint URL (optional)")

    args = parser.parse_args()

    missing = [
        name
        for name, value in (
            ("access key", args.access_key),
            ("secret key", args.secret_key),
            ("account id", args.account_id),
            ("bucket", args.bucket),
        )
        if not value
    ]
    if missing:
        parser.error("Missing parameters: " + ", ".join(missing))

    try:
        deploy(
            Path(args.source).resolve(),
            args.access_key,
            args.secret_key,
            args.account_id,
            args.bucket,
            args.remote_prefix,
            args.endpoint,
        )
    except Exception as exc:
        raise SystemExit(str(exc)) from exc


if __name__ == "__main__":
    main()
