#!/usr/bin/env python3
"""
File: yi_mcuboot_package.py
Function: Wrap an MCUboot-formatted image in the legacy 64-byte upgrade package.
Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

import argparse
import datetime
import struct
from pathlib import Path


PACKAGE_MAGIC = 0x55504744
HEADER_VERSION = 1
FIRMWARE_TYPE_APPLICATION = 2
HEADER_FORMAT = "<IHHHBBIIIHHHH12s9s3sII"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


def crc16_modbus(data: bytes) -> int:
    """Return the CRC-16/MODBUS value for data."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def parse_version(text: str) -> tuple[int, int, int, int]:
    """Parse major.minor.patch[.build] into the package representation."""
    parts = [int(part) for part in text.split(".")]
    if len(parts) not in (3, 4) or any(part < 0 or part > 0xFFFF for part in parts):
        raise ValueError("version must be major.minor.patch[.build], each <= 65535")
    return tuple(parts + [0] * (4 - len(parts)))


def fixed_text(text: str, size: int) -> bytes:
    """Encode a null-padded fixed-width ASCII field."""
    encoded = text.encode("ascii")
    if len(encoded) >= size:
        raise ValueError(f"text is too long for {size}-byte field: {text}")
    return encoded.ljust(size, b"\0")


def build_package(image: bytes, hardware_id: int, version: str) -> bytes:
    """Build the validated legacy header followed by the MCUboot image."""
    major, minor, patch, build = parse_version(version)
    now = datetime.datetime.now()
    values = (
        PACKAGE_MAGIC,
        HEADER_VERSION,
        HEADER_SIZE,
        hardware_id,
        FIRMWARE_TYPE_APPLICATION,
        0,
        HEADER_SIZE,
        len(image),
        crc16_modbus(image),
        major,
        minor,
        patch,
        build,
        fixed_text(now.strftime("%Y-%m-%d"), 12),
        fixed_text(now.strftime("%H:%M:%S"), 9),
        b"\0\0\0",
        0,
        0,
    )
    header_without_crc = struct.pack(HEADER_FORMAT, *values)
    values = values[:-1] + (crc16_modbus(header_without_crc[:-4]),)
    return struct.pack(HEADER_FORMAT, *values) + image


def main() -> None:
    """Parse command-line arguments and write one upgrade package."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--hardware-id", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--version", required=True)
    args = parser.parse_args()

    image = args.input.read_bytes()
    package = build_package(image, args.hardware_id, args.version)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(package)


if __name__ == "__main__":
    main()
