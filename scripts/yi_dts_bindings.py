#!/usr/bin/env python3
"""Load YiCore bindings and validate a parsed YiCore Devicetree."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from yi_dts_parser import DtsCells, DtsNode, DtsReference, DtsTree, parse_file


class BindingError(ValueError):
    """Raised when a binding or a bound DTS node is invalid."""


@dataclass(frozen=True)
class PropertySpec:
    type: str
    required: bool = False
    default: Any = None
    has_default: bool = False


@dataclass(frozen=True)
class Binding:
    compatible: str
    driver: str
    header: str
    define: str
    properties: dict[str, PropertySpec]
    source: Path


@dataclass(frozen=True)
class ValidatedNode:
    node: DtsNode
    binding: Binding
    properties: dict[str, Any]


_COMMON_PROPERTIES = {"compatible", "status", "init-level", "init-priority"}
_VALID_TYPES = {"string", "identifier", "int", "boolean", "phandle"}
_INIT_LEVELS = {"pre-kernel", "post-kernel", "application"}


def _strip_yaml_comment(text: str) -> str:
    quote: str | None = None
    escaped = False
    for index, char in enumerate(text):
        if quote is not None:
            if char == quote and not escaped:
                quote = None
            escaped = (char == "\\") and not escaped
            if char != "\\":
                escaped = False
        elif char in {'"', "'"}:
            quote = char
        elif char == "#":
            return text[:index]
    return text


def _yaml_scalar(text: str, source: Path, line: int) -> Any:
    text = text.strip()
    if not text:
        return {}
    if text.startswith('"'):
        try:
            return json.loads(text)
        except json.JSONDecodeError as exc:
            raise BindingError(f"{source}:{line}: invalid quoted string") from exc
    if text.startswith("'"):
        if len(text) < 2 or not text.endswith("'"):
            raise BindingError(f"{source}:{line}: unterminated quoted string")
        return text[1:-1].replace("''", "'")
    lowered = text.lower()
    if lowered in {"true", "false"}:
        return lowered == "true"
    if lowered in {"null", "~"}:
        return None
    if re.fullmatch(r"-?(?:0[xX][0-9a-fA-F]+|[0-9]+)", text):
        return int(text, 0)
    if text.startswith(("[", "- ", "&", "*", "!", "|", ">")):
        raise BindingError(
            f"{source}:{line}: unsupported YAML construct; YiCore bindings use mappings only"
        )
    return text


def _load_yaml_mapping(path: Path) -> dict[str, Any]:
    root: dict[str, Any] = {}
    stack: list[tuple[int, dict[str, Any]]] = [(-1, root)]

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if "\t" in raw_line[:len(raw_line) - len(raw_line.lstrip())]:
            raise BindingError(f"{path}:{line_number}: tabs are not allowed for indentation")
        content = _strip_yaml_comment(raw_line).rstrip()
        if not content.strip():
            continue
        indent = len(content) - len(content.lstrip(" "))
        body = content.strip()
        if ":" not in body:
            raise BindingError(f"{path}:{line_number}: expected 'key: value'")
        key, scalar = body.split(":", 1)
        key = key.strip()
        if not re.fullmatch(r"[A-Za-z0-9_,.-]+", key):
            raise BindingError(f"{path}:{line_number}: invalid mapping key {key!r}")

        while stack[-1][0] >= indent:
            stack.pop()
        mapping = stack[-1][1]
        if key in mapping:
            raise BindingError(f"{path}:{line_number}: duplicate key {key!r}")
        value = _yaml_scalar(scalar, path, line_number)
        mapping[key] = value
        if isinstance(value, dict):
            stack.append((indent, value))

    return root


def load_binding(path: str | Path) -> Binding:
    source = Path(path)
    raw = _load_yaml_mapping(source)
    required_keys = {"compatible", "driver", "header", "define", "properties"}
    missing = required_keys - raw.keys()
    if missing:
        raise BindingError(f"{source}: missing binding keys: {', '.join(sorted(missing))}")
    unknown = raw.keys() - required_keys
    if unknown:
        raise BindingError(f"{source}: unknown binding keys: {', '.join(sorted(unknown))}")
    if not all(isinstance(raw[key], str) for key in required_keys - {"properties"}):
        raise BindingError(f"{source}: compatible, driver, header and define must be strings")
    if not isinstance(raw["properties"], dict):
        raise BindingError(f"{source}: properties must be a mapping")

    properties: dict[str, PropertySpec] = {}
    for name, definition in raw["properties"].items():
        if not isinstance(definition, dict):
            raise BindingError(f"{source}: property {name!r} must be a mapping")
        allowed = {"type", "required", "default"}
        extra = definition.keys() - allowed
        if extra:
            raise BindingError(
                f"{source}: property {name!r} has unknown keys: {', '.join(sorted(extra))}"
            )
        property_type = definition.get("type")
        if property_type not in _VALID_TYPES:
            raise BindingError(
                f"{source}: property {name!r} has unsupported type {property_type!r}"
            )
        required = definition.get("required", False)
        if not isinstance(required, bool):
            raise BindingError(f"{source}: property {name!r} required must be boolean")
        has_default = "default" in definition
        if required and has_default:
            raise BindingError(f"{source}: property {name!r} cannot be required and have a default")
        properties[name] = PropertySpec(
            type=property_type,
            required=required,
            default=definition.get("default"),
            has_default=has_default,
        )

    return Binding(
        compatible=raw["compatible"],
        driver=raw["driver"],
        header=raw["header"],
        define=raw["define"],
        properties=properties,
        source=source,
    )


def load_bindings(directory: str | Path) -> dict[str, Binding]:
    root = Path(directory)
    bindings: dict[str, Binding] = {}
    for path in sorted(root.rglob("*.yaml")):
        binding = load_binding(path)
        if binding.compatible in bindings:
            raise BindingError(
                f"duplicate compatible {binding.compatible!r} in "
                f"{bindings[binding.compatible].source} and {path}"
            )
        bindings[binding.compatible] = binding
    if not bindings:
        raise BindingError(f"no .yaml bindings found in {root}")
    return bindings


def _normalize_value(node: DtsNode, name: str, value: Any, expected: str) -> Any:
    prefix = f"{node.path}: property {name!r}"
    if expected == "boolean":
        if type(value) is not bool:
            raise BindingError(f"{prefix} must be a boolean property")
        return value
    if expected == "string":
        if not isinstance(value, str):
            raise BindingError(f"{prefix} must be a string")
        return value
    if expected == "identifier":
        if not isinstance(value, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value):
            raise BindingError(f"{prefix} must be a C identifier string")
        return value
    if not isinstance(value, DtsCells) or len(value.values) != 1:
        raise BindingError(f"{prefix} must contain exactly one '<...>' cell")
    item = value.values[0]
    if expected == "int" and type(item) is int:
        return item
    if expected == "phandle" and isinstance(item, DtsReference):
        return item
    raise BindingError(f"{prefix} must be {expected}")


def validate_tree(tree: DtsTree, bindings: dict[str, Binding]) -> list[ValidatedNode]:
    bound_nodes = [
        node for node in tree.root.walk()
        if node is not tree.root and "compatible" in node.properties
    ]
    validated_by_label: dict[str, ValidatedNode] = {}
    disabled_labels: set[str] = set()

    for node in bound_nodes:
        status = node.properties.get("status", "okay")
        if status not in {"okay", "available", "disabled"}:
            raise BindingError(
                f"{node.path}: status must be 'okay', 'available', or 'disabled'"
            )
        if status == "disabled":
            if node.label is not None:
                disabled_labels.add(node.label)
            continue

        compatible_value = node.properties["compatible"]
        compatibles = compatible_value if isinstance(compatible_value, list) else [compatible_value]
        if not all(isinstance(item, str) for item in compatibles):
            raise BindingError(f"{node.path}: compatible must contain strings")
        binding = next((bindings[item] for item in compatibles if item in bindings), None)
        if binding is None:
            raise BindingError(
                f"{node.path}: no binding for compatible {compatible_value!r}"
            )

        unknown = node.properties.keys() - binding.properties.keys() - _COMMON_PROPERTIES
        if unknown:
            raise BindingError(
                f"{node.path}: properties not declared by {binding.source.name}: "
                f"{', '.join(sorted(unknown))}"
            )

        properties: dict[str, Any] = {}
        for name, spec in binding.properties.items():
            if name in node.properties:
                properties[name] = _normalize_value(node, name, node.properties[name], spec.type)
            elif spec.required:
                raise BindingError(f"{node.path}: missing required property {name!r}")
            elif spec.has_default:
                properties[name] = spec.default
            elif spec.type == "boolean":
                properties[name] = False

        init_level = properties.get(
            "init-level", node.properties.get("init-level", "application")
        )
        if init_level not in _INIT_LEVELS:
            raise BindingError(f"{node.path}: invalid init-level {init_level!r}")
        if "init-priority" in properties:
            priority = properties["init-priority"]
        else:
            priority_value = node.properties.get("init-priority", DtsCells((50,)))
            priority = _normalize_value(node, "init-priority", priority_value, "int")
        if not 0 <= priority <= 99:
            raise BindingError(f"{node.path}: init-priority must be in range 0..99")
        properties["init-level"] = init_level
        properties["init-priority"] = priority
        properties["status"] = status

        if node.label is None:
            if status == "okay":
                raise BindingError(f"{node.path}: enabled device requires a label")
            continue
        validated_by_label[node.label] = ValidatedNode(
            node=node, binding=binding, properties=properties
        )

    selected: set[str] = set()

    def select(label: str, owner: DtsNode | None = None) -> None:
        if label in selected:
            return
        try:
            item = validated_by_label[label]
        except KeyError as exc:
            prefix = owner.path if owner is not None else tree.root.path
            if label in disabled_labels:
                raise BindingError(f"{prefix}: references disabled node {label!r}") from exc
            raise BindingError(
                f"{prefix}: references node {label!r} that does not generate a device"
            ) from exc
        selected.add(label)
        for value in item.properties.values():
            if isinstance(value, DtsReference):
                select(value.label, item.node)

    for item in validated_by_label.values():
        if item.properties["status"] == "okay":
            select(item.node.label)

    return [
        validated_by_label[node.label]
        for node in bound_nodes
        if node.label in selected
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dts", type=Path)
    parser.add_argument("--bindings", type=Path, required=True)
    args = parser.parse_args()
    try:
        nodes = validate_tree(parse_file(args.dts), load_bindings(args.bindings))
    except (OSError, ValueError) as exc:
        parser.exit(1, f"error: {exc}\n")
    print(json.dumps([
        {
            "label": item.node.label,
            "path": item.node.path,
            "compatible": item.binding.compatible,
            "driver": item.binding.driver,
            "properties": {
                key: ({"$ref": value.label} if isinstance(value, DtsReference) else value)
                for key, value in item.properties.items()
            },
        }
        for item in nodes
    ], ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
