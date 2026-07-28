#!/usr/bin/env python3
"""Manage reproducible multi-repository YiCore workspaces.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

import yaml


class ManifestError(ValueError):
    """Report an invalid manifest or unsafe workspace update."""


def load_manifest(path: Path) -> dict[str, Any]:
    """Load and validate a YiCore workspace manifest.

    Args:
        path: YAML manifest path.
    Returns:
        Validated top-level manifest mapping.
    Raises:
        ManifestError: Syntax, schema or project paths are invalid.
    """

    try:
        document = yaml.safe_load(path.read_text(encoding="utf-8"))
    except (OSError, yaml.YAMLError) as error:
        raise ManifestError(f"cannot load manifest: {path}") from error
    if not isinstance(document, dict):
        raise ManifestError("manifest must be a mapping")
    manifest = document.get("manifest")
    if not isinstance(manifest, dict):
        raise ManifestError("missing manifest mapping")
    projects = manifest.get("projects", [])
    if not isinstance(projects, list):
        raise ManifestError("manifest projects must be a list")

    seen_names: set[str] = set()
    seen_paths: set[str] = set()
    for index, project in enumerate(projects, 1):
        if not isinstance(project, dict):
            raise ManifestError(f"project {index} must be a mapping")
        for field in ("name", "url", "revision", "path"):
            if not isinstance(project.get(field), str) or not project[field]:
                raise ManifestError(f"project {index} missing {field}")
        project_path = Path(project["path"])
        if project_path.is_absolute() or ".." in project_path.parts:
            raise ManifestError(
                f"project {project['name']} path must stay in workspace"
            )
        if project["name"] in seen_names:
            raise ManifestError(f"duplicate project name: {project['name']}")
        normalized_path = project_path.as_posix().lower()
        if normalized_path in seen_paths:
            raise ManifestError(f"duplicate project path: {project['path']}")
        seen_names.add(project["name"])
        seen_paths.add(normalized_path)
        if "optional" in project and not isinstance(
            project["optional"], bool
        ):
            raise ManifestError(
                f"project {project['name']} optional must be boolean"
            )
    return manifest


def _git_output(arguments: list[str], cwd: Path) -> str:
    """Run a read-only Git query and return stripped standard output."""

    try:
        result = subprocess.run(
            ["git", *arguments],
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        raise ManifestError(
            f"git query failed in {cwd}: {' '.join(arguments)}"
        ) from error
    return result.stdout.strip()


def update_workspace(
    workspace: Path,
    manifest: dict[str, Any],
    include_optional: bool = False,
) -> list[str]:
    """Clone or update projects to manifest revisions.

    Args:
        workspace: Workspace top directory.
        manifest: Validated manifest mapping.
        include_optional: Also materialize optional projects.
    Returns:
        Updated project names.
    Side effects:
        Clones repositories, fetches revisions and checks out detached commits.
    Raises:
        ManifestError: A repository is dirty or Git cannot update it.
    """

    workspace = workspace.resolve()
    workspace.mkdir(parents=True, exist_ok=True)
    updated: list[str] = []
    for project in manifest.get("projects", []):
        if project.get("optional", False) and not include_optional:
            continue
        destination = workspace / project["path"]
        if destination.exists():
            if not (destination / ".git").exists():
                raise ManifestError(
                    f"project path is not a Git repository: {destination}"
                )
            if _git_output(["status", "--porcelain"], destination):
                raise ManifestError(
                    f"refusing to update dirty project: {project['name']}"
                )
            commands = (
                ["remote", "set-url", "origin", project["url"]],
                ["fetch", "--tags", "origin", project["revision"]],
                ["checkout", "--detach", "FETCH_HEAD"],
            )
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            commands = (
                [
                    "clone",
                    "--no-checkout",
                    project["url"],
                    str(destination),
                ],
                ["-C", str(destination), "fetch", "origin", project["revision"]],
                [
                    "-C",
                    str(destination),
                    "checkout",
                    "--detach",
                    "FETCH_HEAD",
                ],
            )
        for command in commands:
            try:
                subprocess.run(["git", *command], check=True)
            except (OSError, subprocess.CalledProcessError) as error:
                raise ManifestError(
                    f"failed to update project: {project['name']}"
                ) from error
        updated.append(project["name"])
    return updated


def freeze_manifest(
    workspace: Path,
    manifest: dict[str, Any],
) -> dict[str, Any]:
    """Replace project revisions with checked-out commit SHAs.

    Args:
        workspace: Workspace top directory.
        manifest: Validated manifest mapping.
    Returns:
        New YAML-serializable document containing frozen revisions.
    """

    frozen_projects: list[dict[str, Any]] = []
    for project in manifest.get("projects", []):
        checkout = workspace.resolve() / project["path"]
        if not (checkout / ".git").exists():
            if project.get("optional", False):
                continue
            raise ManifestError(
                f"project is not checked out: {project['name']}"
            )
        frozen = dict(project)
        frozen["revision"] = _git_output(["rev-parse", "HEAD"], checkout)
        frozen_projects.append(frozen)
    return {
        "manifest": {
            "version": manifest.get("version", "1.0"),
            "projects": frozen_projects,
        }
    }


def write_manifest(document: dict[str, Any], output: Path) -> None:
    """Write a deterministic YAML manifest document.

    Args:
        document: YAML-serializable manifest document.
        output: Destination path.
    Side effects:
        Creates parent directories and replaces the destination file.
    """

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        yaml.safe_dump(document, sort_keys=False),
        encoding="utf-8",
        newline="\n",
    )
