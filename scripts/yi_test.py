#!/usr/bin/env python3
"""Discover and validate Twister-style YiCore test manifests.

Author: Don
Date: 2026-08-02
Version: 1.0.0
"""

from __future__ import annotations

from pathlib import Path

import yaml


class TestManifestError(ValueError):
    """Report an invalid YiCore test manifest."""


def discover_tests(root: Path) -> list[dict]:
    """Return normalized scenarios from testcase.yaml files below root."""

    scenarios: list[dict] = []
    for manifest in sorted(root.rglob("testcase.yaml")):
        content = yaml.safe_load(manifest.read_text(encoding="utf-8")) or {}
        tests = content.get("tests")
        if not isinstance(tests, dict) or not tests:
            raise TestManifestError(f"{manifest}: missing tests mapping")
        for name, raw in tests.items():
            if not isinstance(raw, dict):
                raise TestManifestError(f"{manifest}: {name} must be a mapping")
            platforms = raw.get("platform_allow", [])
            if isinstance(platforms, str):
                platforms = [platforms]
            if not isinstance(platforms, list) or not all(
                isinstance(item, str) for item in platforms
            ):
                raise TestManifestError(
                    f"{manifest}: {name} platform_allow must be a list"
                )
            scenarios.append({
                "name": name,
                "path": manifest.parent,
                "platform_allow": platforms,
                "build_only": bool(raw.get("build_only", False)),
                "tags": list(raw.get("tags", [])),
            })
    return scenarios
