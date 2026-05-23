#!/usr/bin/env python3
"""Export an NCBB JSON bundle as the frozen NCBB binary format."""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from typing import Any


OPCODES = {
    "LABEL": 0,
    "PUSH_CONST": 1,
    "LOAD_NAME": 2,
    "STORE_NAME": 3,
    "POP": 4,
    "BUILD_LIST": 5,
    "BUILD_MAP": 6,
    "INDEX_GET": 7,
    "INDEX_SET": 8,
    "UNARY_NEG": 9,
    "BINARY_ADD": 10,
    "BINARY_SUB": 11,
    "BINARY_MUL": 12,
    "BINARY_DIV": 13,
    "BINARY_MOD": 14,
    "COMPARE_EQ": 15,
    "COMPARE_NE": 16,
    "COMPARE_GT": 17,
    "COMPARE_LT": 18,
    "COMPARE_GE": 19,
    "COMPARE_LE": 20,
    "CALL": 21,
    "JUMP": 22,
    "JUMP_IF_FALSE": 23,
    "RETURN": 24,
    "UNARY_NOT": 25,
}

NCBB_MAGIC = b"NCBB"
NCBB_VERSION = 1
NCBB_HEADER_SIZE = 36
BUILTIN_PREFIX = "builtin."


def _encode_u32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def _encode_u16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def _encode_i64(value: int) -> bytes:
    return struct.pack("<q", int(value))


def _serialize_payload(payload: dict[str, Any]) -> bytes:
    functions = sorted(payload.get("functions", {}).items())
    key_indices = {key: index for index, (key, _fn) in enumerate(functions)}
    public_indices = {
        f"{fn.get('module', '')}.{fn.get('name', '')}": index
        for index, (_key, fn) in enumerate(functions)
    }

    string_ids: dict[str, int] = {}
    strings: list[str] = []
    builtin_ids: dict[str, int] = {}
    builtins: list[str] = []
    function_rows: list[dict[str, Any]] = []
    code_blobs: list[bytes] = []

    def add_string(value: str) -> int:
        if value not in string_ids:
            string_ids[value] = len(strings)
            strings.append(value)
        return string_ids[value]

    def add_builtin(value: str) -> int:
        if value not in builtin_ids:
            builtin_ids[value] = len(builtins)
            builtins.append(value)
        return builtin_ids[value]

    for key, fn in functions:
        slot_names: list[str] = []
        for param in fn.get("params", []):
            if param not in slot_names:
                slot_names.append(str(param))

        code = bytearray()
        for ins in fn.get("code", []):
            if not ins:
                continue
            op = ins[0]
            if op in ("LABEL", "JUMP", "JUMP_IF_FALSE"):
                sid = add_string(str(ins[1]))
                code.append(OPCODES[op])
                code += _encode_u32(sid)
                continue

            if op == "PUSH_CONST":
                value = ins[1] if len(ins) > 1 else None
                code.append(OPCODES[op])
                if value is None:
                    sid = add_string("")
                    code.append(2)
                    code += _encode_u32(sid)
                elif isinstance(value, bool):
                    code.append(1)
                    code.append(1 if value else 0)
                elif isinstance(value, int):
                    code.append(0)
                    code += _encode_i64(value)
                elif isinstance(value, str):
                    sid = add_string(value)
                    code.append(2)
                    code += _encode_u32(sid)
                else:
                    raise TypeError(f"Unsupported PUSH_CONST value in {key}: {value!r}")
                continue

            if op in ("LOAD_NAME", "STORE_NAME"):
                name = str(ins[1])
                if name not in slot_names:
                    slot_names.append(name)
                code.append(OPCODES[op])
                code += _encode_u16(slot_names.index(name))
                continue

            if op == "CALL":
                target = ins[1]
                argc = int(ins[2]) if len(ins) > 2 else 0
                if isinstance(target, str) and target.startswith(BUILTIN_PREFIX):
                    target_id = 0x80000000 | add_builtin(target)
                elif isinstance(target, str):
                    if target in public_indices:
                        target_id = public_indices[target]
                    elif target in key_indices:
                        target_id = key_indices[target]
                    else:
                        short_name = target.split(".")[-1]
                        builtin_name = f"{BUILTIN_PREFIX}{short_name}"
                        target_id = 0x80000000 | add_builtin(builtin_name)
                else:
                    target_id = int(target)
                code.append(OPCODES[op])
                code += _encode_u32(target_id)
                code.append(argc & 0xFF)
                continue

            if op in ("BUILD_LIST", "BUILD_MAP"):
                code.append(OPCODES[op])
                code.append(int(ins[1]) & 0xFF)
                continue

            if op not in OPCODES:
                raise ValueError(f"Unsupported opcode in {key}: {op!r}")
            code.append(OPCODES[op])

        name_id = add_string(key)
        for slot_name in slot_names:
            add_string(slot_name)

        function_rows.append(
            {
                "name_id": name_id,
                "n_params": len(fn.get("params", [])),
                "n_slots": len(slot_names),
                "slot_names": list(slot_names),
                "code": bytes(code),
            }
        )
        code_blobs.append(bytes(code))

    fn_table = bytearray()
    code_cursor = 0
    for row in function_rows:
        fn_table += struct.pack(
            "<IHHII",
            row["name_id"],
            row["n_params"],
            row["n_slots"],
            code_cursor,
            len(row["code"]),
        )
        for slot_name in row["slot_names"]:
            fn_table += _encode_u32(string_ids[slot_name])
        code_cursor += len(row["code"])

    builtin_blob = b"".join(name.encode("utf-8") + b"\0" for name in builtins)
    string_blob = b"".join(name.encode("utf-8") + b"\0" for name in strings)
    code_blob = b"".join(code_blobs)

    off_fn = NCBB_HEADER_SIZE
    off_bi = off_fn + len(fn_table)
    off_str = off_bi + len(builtin_blob)
    off_code = off_str + len(string_blob)

    header = bytearray()
    header += NCBB_MAGIC
    header += struct.pack("<I", NCBB_VERSION)
    header += struct.pack("<I", len(function_rows))
    header += struct.pack("<I", len(strings))
    header += struct.pack("<I", len(builtins))
    header += struct.pack("<I", off_fn)
    header += struct.pack("<I", off_bi)
    header += struct.pack("<I", off_str)
    header += struct.pack("<I", off_code)

    return bytes(header + fn_table + builtin_blob + string_blob + code_blob)


def export_ncbb_as_bin(input_path: Path, output_path: Path) -> None:
    payload = json.loads(input_path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise RuntimeError("NCBB input must be a JSON object")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(_serialize_payload(payload))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Input NCBB JSON bundle")
    parser.add_argument("output", type=Path, help="Output NCBB binary")
    args = parser.parse_args()
    export_ncbb_as_bin(args.input, args.output)


if __name__ == "__main__":
    main()
