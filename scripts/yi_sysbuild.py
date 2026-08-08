#!/usr/bin/env python3
"""Load a deterministic bare-metal multi-image build plan.

Author: Don
Date: 2026-08-02
Version: 1.0.0
"""

from __future__ import annotations

from pathlib import Path

import yaml


class SysbuildError(ValueError):
    """Report an invalid multi-image manifest."""


def load_plan(product_root: Path) -> list[dict]:
    """Validate sysbuild.yml and return images in dependency order."""

    manifest = product_root / "sysbuild.yml"
    if not manifest.is_file():
        raise SysbuildError(f"multi-image manifest not found: {manifest}")
    raw = yaml.safe_load(manifest.read_text(encoding="utf-8")) or {}
    images = raw.get("images")
    if not isinstance(images, dict) or "application" not in images:
        raise SysbuildError("sysbuild.yml must define an application image")
    pending = dict(images)
    result: list[dict] = []
    emitted: set[str] = set()
    while pending:
        ready = []
        for name, config in pending.items():
            config = config or {}
            depends = config.get("depends_on", [])
            if isinstance(depends, str):
                depends = [depends]
            unknown = set(depends) - set(images)
            if unknown:
                raise SysbuildError(
                    f"image {name} has unknown dependencies: {', '.join(sorted(unknown))}"
                )
            if set(depends).issubset(emitted):
                ready.append((name, config, depends))
        if not ready:
            raise SysbuildError("multi-image dependency cycle")
        for name, config, depends in ready:
            image_root = product_root / "firmware" / "images" / name
            if not image_root.is_dir():
                raise SysbuildError(f"image directory not found: {image_root}")
            result.append({"name": name, "path": image_root, "depends_on": depends})
            emitted.add(name)
            del pending[name]
    return result
