#!/usr/bin/env python3
"""Add consistent file and function documentation to YiCore-owned sources.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

from __future__ import annotations

import re
from pathlib import Path


# Repository areas maintained by YiCore; imported and generated sources are excluded.
SOURCE_ROOTS = ("core", "drivers", "soc", "subsys", "ports", "scripts")
# Supported source suffixes and their comment syntaxes.
SOURCE_SUFFIXES = {".c", ".h", ".py"}
# Common ownership metadata written into every maintained source header.
FILE_AUTHOR = "Don"
FILE_DATE = "2026-07-26"
FILE_VERSION = "1.0.0"
# C control-flow keywords which can superficially resemble function declarations.
CONTROL_WORDS = {"if", "for", "while", "switch", "return", "sizeof"}


def _summary(path: Path) -> str:
    """Build a concise module description from a source path."""
    stem = path.stem.removeprefix("yi_").replace("_", " ")
    role = "interface" if path.suffix == ".h" else "implementation"
    if path.suffix == ".py":
        role = "utility"
    return f"YiCore {stem} {role}."


def _add_file_header(path: Path, text: str) -> str:
    """Insert a file-level Doxygen comment or Python module docstring."""
    head = "\n".join(text.splitlines()[:15])
    if path.suffix == ".py":
        metadata = (
            f"\n\nAuthor: {FILE_AUTHOR}"
            f"\nDate: {FILE_DATE}"
            f"\nVersion: {FILE_VERSION}"
        )
        module_doc = re.search(
            r'(?s)(?P<open>^[rubf]*""")(?P<body>.*?)(?P<close>""")',
            text.removeprefix(text.split("\n", 1)[0] + "\n")
            if text.startswith("#!") else text,
        )
        if module_doc:
            body = module_doc.group("body").rstrip()
            if "Author:" not in body:
                replacement = module_doc.group("open") + body + metadata + "\n" + module_doc.group("close")
                start = text.find(module_doc.group(0))
                return text[:start] + replacement + text[start + len(module_doc.group(0)):]
            return text
        doc = f'"""{_summary(path)}{metadata}\n"""\n\n'
        if text.startswith("#!"):
            first, rest = text.split("\n", 1)
            return f"{first}\n{doc}{rest}"
        return doc + text
    if "@file" in head:
        if "@author" not in head:
            text = re.sub(
                r"(?m)^(\s*\*\s*@brief[^\n]*\n)",
                rf"\1 * @author {FILE_AUTHOR}\n * @date {FILE_DATE}\n * @version {FILE_VERSION}\n",
                text,
                count=1,
            )
        return text
    header = (
        "/**\n"
        f" * @file {path.name}\n"
        f" * @brief {_summary(path)}\n"
        f" * @author {FILE_AUTHOR}\n"
        f" * @date {FILE_DATE}\n"
        f" * @version {FILE_VERSION}\n"
        " */\n\n"
    )
    return header + text


def _words(identifier: str) -> str:
    """Convert an identifier into readable words for generated descriptions."""
    words = identifier.strip("_").replace("_", " ")
    return words or identifier


def _function_brief(name: str) -> str:
    """Derive an action-oriented function summary from its identifier."""
    readable = _words(name)
    actions = {
        "init": "Initialize", "get": "Get", "set": "Set", "read": "Read",
        "write": "Write", "is": "Check whether", "has": "Check whether",
        "enable": "Enable", "disable": "Disable", "start": "Start",
        "stop": "Stop", "parse": "Parse", "validate": "Validate",
        "find": "Find", "collect": "Collect", "sort": "Sort",
        "configure": "Configure", "transfer": "Transfer",
    }
    parts = name.strip("_").split("_")
    # Skip the conventional yi/module prefix when locating the operation verb.
    for index, part in enumerate(parts):
        if part in actions:
            subject = " ".join(parts[index + 1:]) or "the module"
            return f"{actions[part]} {subject.replace('_', ' ')}."
    return f"Perform the {readable} operation."


def _parameter_names(parameters: str) -> list[str]:
    """Extract parameter identifiers from a simple C parameter list."""
    compact = re.sub(r"/\*.*?\*/", "", parameters, flags=re.S).strip()
    if not compact or compact == "void":
        return []
    names: list[str] = []
    for parameter in compact.split(","):
        parameter = parameter.strip()
        # Function-pointer parameters need the name between (* and ).
        pointer = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", parameter)
        match = pointer or re.search(r"([A-Za-z_]\w*)\s*(?:\[[^]]*\])?\s*$", parameter)
        if match:
            names.append(match.group(1))
    return names


def _parameter_description(name: str) -> str:
    """Return a useful description for a conventional parameter name."""
    descriptions = {
        "dev": "Device instance.", "config": "Device configuration.",
        "cfg": "Device configuration.", "data": "Driver runtime data.",
        "buffer": "Data buffer.", "length": "Number of bytes to process.",
        "offset": "Byte offset from the start of the device.",
        "timeout_ms": "Operation timeout in milliseconds.",
        "callback": "Callback registration object.",
        "handler": "Callback function to invoke.", "pins": "GPIO pin mask.",
        "pin_mask": "GPIO pin mask.", "value": "Value to process.",
        "name": "Registered device name.", "level": "Initialization level.",
    }
    return descriptions.get(name, f"{_words(name).capitalize()} value.")


def _improve_parameter_docs(text: str) -> str:
    """Replace placeholder parameter documentation with semantic descriptions."""
    return re.sub(
        r"(@param\s+)([A-Za-z_]\w*)\s+Function parameter\.",
        lambda match: match.group(1) + match.group(2) + " "
        + _parameter_description(match.group(2)),
        text,
    )


def _document_c_functions(text: str) -> str:
    """Add Doxygen blocks to undocumented C declarations and definitions."""
    pattern = re.compile(
        r"(?m)^(?P<indent>[ \t]*)(?P<prefix>"
        r"(?:static\s+)?(?:inline\s+)?(?:__weak\s+)?"
        r"(?:const\s+)?[A-Za-z_]\w*(?:\s+|\s*\*\s*)"
        r"(?:const\s+)?(?:\*\s*)?)(?P<name>[A-Za-z_]\w*)"
        r"\s*\((?P<params>[^;{}]*?)\)\s*(?P<tail>;|\{)",
    )
    insertions: list[tuple[int, str]] = []
    for match in pattern.finditer(text):
        name = match.group("name")
        if name in CONTROL_WORDS:
            continue
        line_start = match.start()
        before = text[max(0, line_start - 500):line_start].rstrip()
        if before.endswith("*/"):
            continue
        indent = match.group("indent")
        lines = [f"{indent}/**", f"{indent} * @brief {_function_brief(name)}"]
        for parameter in _parameter_names(match.group("params")):
            lines.append(
                f"{indent} * @param {parameter} {_parameter_description(parameter)}"
            )
        lines.append(f"{indent} */\n")
        insertions.append((line_start, "\n".join(lines)))
    for offset, comment in reversed(insertions):
        text = text[:offset] + comment + text[offset:]
    return text


def _document_struct_fields(text: str) -> str:
    """Add trailing documentation to plain fields in structs and unions."""
    block_pattern = re.compile(
        r"(?ms)(?P<head>^(?:typedef\s+)?(?:struct|union)(?:\s+\w+)?\s*\{)"
        r"(?P<body>.*?)(?P<tail>^\}\s*(?:\w+)?\s*;)",
    )

    def document_block(match: re.Match[str]) -> str:
        body = match.group("body")
        field_pattern = re.compile(
            r"(?m)^(?P<line>[ \t]+(?!/|\*|#)[^;{}\n]+\b"
            r"(?P<name>[A-Za-z_]\w*)\s*(?:\[[^]]*\])?;)\s*$"
        )

        def document_field(field: re.Match[str]) -> str:
            line = field.group("line")
            if "(*" in line or "/**<" in line:
                return line
            name = field.group("name")
            return f"{line} /**< {_words(name).capitalize()} value. */"

        return (
            match.group("head") + field_pattern.sub(document_field, body)
            + match.group("tail")
        )

    return block_pattern.sub(document_block, text)


def _source_files(repository: Path) -> list[Path]:
    """Return maintained source files in deterministic path order."""
    files = [
        path
        for root in SOURCE_ROOTS
        for path in (repository / root).rglob("*")
        if path.is_file() and path.suffix in SOURCE_SUFFIXES
    ]
    application = repository / "applications" / "mcuboot-stm32f103" / "Core"
    files.extend(
        path for path in application.rglob("*")
        if path.is_file() and path.suffix in {".c", ".h"} and path.name != "keys.c"
    )
    return sorted(set(files))


def main() -> int:
    """Document all maintained sources and report the number changed."""
    repository = Path(__file__).resolve().parents[1]
    changed = 0
    for path in _source_files(repository):
        original = path.read_text(encoding="utf-8")
        updated = _add_file_header(path, original)
        if path.suffix in {".c", ".h"}:
            updated = _document_c_functions(updated)
            updated = _improve_parameter_docs(updated)
            updated = _document_struct_fields(updated)
        if updated != original:
            path.write_text(updated, encoding="utf-8", newline="\n")
            changed += 1
    print(f"documented {changed} source files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
