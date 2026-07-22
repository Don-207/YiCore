#!/usr/bin/env python3
"""Small, dependency-free parser for the YiCore Devicetree subset."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterator


class DtsParseError(ValueError):
    """Raised when a DTS source is syntactically or semantically invalid."""


@dataclass(frozen=True)
class DtsReference:
    label: str


@dataclass(frozen=True)
class DtsCells:
    values: tuple[Any, ...]


@dataclass
class DtsNode:
    name: str
    label: str | None = None
    properties: dict[str, Any] = field(default_factory=dict)
    children: list["DtsNode"] = field(default_factory=list)
    parent: "DtsNode | None" = field(default=None, repr=False)

    @property
    def path(self) -> str:
        if self.parent is None:
            return "/"
        prefix = self.parent.path.rstrip("/")
        return f"{prefix}/{self.name}"

    def walk(self) -> Iterator["DtsNode"]:
        yield self
        for child in self.children:
            yield from child.walk()


@dataclass
class DtsTree:
    root: DtsNode
    labels: dict[str, DtsNode]

    def node_by_label(self, label: str) -> DtsNode:
        try:
            return self.labels[label]
        except KeyError as exc:
            raise KeyError(f"unknown DTS label: {label}") from exc

    def to_dict(self) -> dict[str, Any]:
        def encode(value: Any) -> Any:
            if isinstance(value, DtsReference):
                return {"$ref": value.label}
            if isinstance(value, DtsCells):
                return {"$cells": [encode(item) for item in value.values]}
            if isinstance(value, list):
                return [encode(item) for item in value]
            return value

        def encode_node(node: DtsNode) -> dict[str, Any]:
            result: dict[str, Any] = {
                "name": node.name,
                "path": node.path,
                "properties": {
                    key: encode(value) for key, value in node.properties.items()
                },
                "children": [encode_node(child) for child in node.children],
            }
            if node.label is not None:
                result["label"] = node.label
            return result

        return encode_node(self.root)


@dataclass(frozen=True)
class _Token:
    kind: str
    value: Any
    line: int
    column: int


class _Lexer:
    _punctuation = {
        "{": "LBRACE", "}": "RBRACE", ";": "SEMI", "=": "EQUAL",
        "<": "LANGLE", ">": "RANGLE", ":": "COLON", ",": "COMMA",
        "&": "AMP", "[": "LBRACKET", "]": "RBRACKET",
    }

    def __init__(self, text: str, source: str) -> None:
        self.text = text
        self.source = source
        self.index = 0
        self.line = 1
        self.column = 1

    def _error(self, message: str) -> DtsParseError:
        return DtsParseError(f"{self.source}:{self.line}:{self.column}: {message}")

    def _advance(self) -> str:
        char = self.text[self.index]
        self.index += 1
        if char == "\n":
            self.line += 1
            self.column = 1
        else:
            self.column += 1
        return char

    def _skip_space_and_comments(self) -> None:
        while self.index < len(self.text):
            if self.text[self.index].isspace():
                self._advance()
                continue
            if self.text.startswith("//", self.index):
                while self.index < len(self.text) and self._advance() != "\n":
                    pass
                continue
            if self.text.startswith("/*", self.index):
                self._advance()
                self._advance()
                while self.index < len(self.text) and not self.text.startswith("*/", self.index):
                    self._advance()
                if self.index >= len(self.text):
                    raise self._error("unterminated block comment")
                self._advance()
                self._advance()
                continue
            break

    def tokens(self) -> Iterator[_Token]:
        delimiters = set(self._punctuation) | {'"'}
        while True:
            self._skip_space_and_comments()
            if self.index >= len(self.text):
                yield _Token("EOF", None, self.line, self.column)
                return

            line, column = self.line, self.column
            char = self.text[self.index]
            if char in self._punctuation:
                self._advance()
                yield _Token(self._punctuation[char], char, line, column)
                continue

            if char == '"':
                start = self.index
                self._advance()
                escaped = False
                while self.index < len(self.text):
                    current = self._advance()
                    if current == '"' and not escaped:
                        break
                    escaped = (current == "\\") and not escaped
                    if current != "\\":
                        escaped = False
                else:
                    raise self._error("unterminated string")
                raw = self.text[start:self.index]
                try:
                    value = json.loads(raw)
                except json.JSONDecodeError as exc:
                    raise self._error(f"invalid string escape: {exc.msg}") from exc
                yield _Token("STRING", value, line, column)
                continue

            start = self.index
            while (self.index < len(self.text) and
                   not self.text[self.index].isspace() and
                   self.text[self.index] not in delimiters):
                self._advance()
            word = self.text[start:self.index]
            if not word:
                raise self._error(f"unexpected character {char!r}")
            if re.fullmatch(r"-?(?:0[xX][0-9a-fA-F]+|[0-9]+)", word):
                yield _Token("NUMBER", int(word, 0), line, column)
            else:
                yield _Token("IDENT", word, line, column)


class _Parser:
    def __init__(self, text: str, source: str) -> None:
        self.source = source
        self.tokens = list(_Lexer(text, source).tokens())
        self.index = 0
        self.labels: dict[str, DtsNode] = {}

    @property
    def token(self) -> _Token:
        return self.tokens[min(self.index, len(self.tokens) - 1)]

    def _error(self, message: str, token: _Token | None = None) -> DtsParseError:
        current = token or self.token
        return DtsParseError(
            f"{self.source}:{current.line}:{current.column}: {message}"
        )

    def _accept(self, kind: str) -> _Token | None:
        if self.token.kind != kind:
            return None
        token = self.token
        self.index += 1
        return token

    def _expect(self, kind: str, description: str | None = None) -> _Token:
        token = self._accept(kind)
        if token is None:
            expected = description or kind
            raise self._error(f"expected {expected}, found {self.token.value!r}")
        return token

    def parse(self) -> DtsTree:
        while self.token.kind == "IDENT" and self.token.value.startswith("/dts-"):
            self.index += 1
            self._expect("SEMI", "';'")

        root = self._parse_node(parent=None)
        if root.name != "/":
            raise self._error("root node must be '/'")
        while self.token.kind != "EOF":
            if self.token.kind == "AMP":
                self._parse_overlay()
                continue
            raise self._error("expected a '&label' overlay or end of file")
        self._expect("EOF", "end of file")
        self._validate_references(root)
        return DtsTree(root=root, labels=self.labels)

    def _parse_overlay(self) -> None:
        self._expect("AMP")
        label = self._expect("IDENT", "overlay label").value
        try:
            node = self.labels[label]
        except KeyError as exc:
            raise self._error(f"overlay references unknown label {label!r}") from exc
        self._expect("LBRACE", "'{'")
        while self.token.kind != "RBRACE":
            if self.token.kind == "EOF":
                raise self._error(f"unterminated overlay {label!r}")
            self._parse_member(node, allow_override=True)
        self._expect("RBRACE")
        self._expect("SEMI", "';'")

    def _parse_node(self, parent: DtsNode | None) -> DtsNode:
        first = self._expect("IDENT", "node name or label")
        label: str | None = None
        name = first.value
        if self._accept("COLON") is not None:
            label = name
            name = self._expect("IDENT", "node name").value

        self._expect("LBRACE", "'{'")
        node = DtsNode(name=name, label=label, parent=parent)
        if label is not None:
            if label in self.labels:
                raise self._error(f"duplicate label {label!r}", first)
            self.labels[label] = node

        while self.token.kind != "RBRACE":
            if self.token.kind == "EOF":
                raise self._error(f"unterminated node {name!r}")
            self._parse_member(node)
        self._expect("RBRACE")
        self._expect("SEMI", "';'")
        return node

    def _parse_member(self, node: DtsNode, allow_override: bool = False) -> None:
        first = self._expect("IDENT", "property or node name")
        if self.token.kind == "COLON":
            self.index -= 1
            child = self._parse_node(node)
            node.children.append(child)
            return
        if self.token.kind == "LBRACE":
            self.index -= 1
            child = self._parse_node(node)
            node.children.append(child)
            return

        name = first.value
        if name in node.properties and not allow_override:
            raise self._error(f"duplicate property {name!r}", first)
        if self._accept("SEMI") is not None:
            node.properties[name] = True
            return

        self._expect("EQUAL", "'='")
        node.properties[name] = self._parse_value()
        self._expect("SEMI", "';'")

    def _parse_value(self) -> Any:
        values = [self._parse_atom()]
        while self._accept("COMMA") is not None:
            values.append(self._parse_atom())
        return values[0] if len(values) == 1 else values

    def _parse_atom(self) -> Any:
        if token := self._accept("STRING"):
            return token.value
        if token := self._accept("NUMBER"):
            return token.value
        if token := self._accept("IDENT"):
            return token.value
        if self._accept("AMP") is not None:
            return DtsReference(self._expect("IDENT", "phandle label").value)
        if self._accept("LANGLE") is not None:
            cells: list[Any] = []
            while self.token.kind != "RANGLE":
                if self.token.kind in {"EOF", "SEMI"}:
                    raise self._error("unterminated cell list")
                if self._accept("AMP") is not None:
                    cells.append(DtsReference(self._expect("IDENT", "phandle label").value))
                elif token := self._accept("NUMBER"):
                    cells.append(token.value)
                elif token := self._accept("IDENT"):
                    cells.append(token.value)
                else:
                    raise self._error("expected a number, identifier, or phandle in '<...>'")
            self._expect("RANGLE")
            return DtsCells(tuple(cells))
        raise self._error("expected property value")

    def _validate_references(self, root: DtsNode) -> None:
        def references(value: Any) -> Iterator[DtsReference]:
            if isinstance(value, DtsReference):
                yield value
            elif isinstance(value, DtsCells):
                for item in value.values:
                    yield from references(item)
            elif isinstance(value, list):
                for item in value:
                    yield from references(item)

        for node in root.walk():
            for value in node.properties.values():
                for reference in references(value):
                    if reference.label not in self.labels:
                        raise self._error(
                            f"node {node.path!r} references unknown label "
                            f"{reference.label!r}"
                        )


def parse_text(text: str, source: str = "<string>") -> DtsTree:
    return _Parser(text, source).parse()


def parse_file(path: str | Path) -> DtsTree:
    source = Path(path)
    return parse_text(_expand_includes(source, ()), str(source))


def _expand_includes(source: Path, stack: tuple[Path, ...]) -> str:
    resolved = source.resolve()
    if resolved in stack:
        chain = " -> ".join(str(path) for path in (*stack, resolved))
        raise DtsParseError(f"DTS include cycle: {chain}")

    text = source.read_text(encoding="utf-8")
    pattern = re.compile(r'^\s*/include/\s+"([^"]+)"\s*;?\s*$', re.MULTILINE)

    def replace(match: re.Match[str]) -> str:
        included = (source.parent / match.group(1)).resolve()
        return _expand_includes(included, (*stack, resolved))

    return pattern.sub(replace, text)


def main() -> int:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("dts", type=Path, help="DTS source file")
    argument_parser.add_argument(
        "--pretty", action="store_true", help="pretty-print parsed JSON"
    )
    args = argument_parser.parse_args()

    try:
        tree = parse_file(args.dts)
    except (OSError, DtsParseError) as exc:
        argument_parser.exit(1, f"error: {exc}\n")

    print(json.dumps(
        tree.to_dict(),
        ensure_ascii=False,
        indent=2 if args.pretty else None,
    ))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
