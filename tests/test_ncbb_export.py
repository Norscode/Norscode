from __future__ import annotations

import struct
from pathlib import Path

from tools.export_ncbb_as_bin import _serialize_payload


def _read_pool(blob: bytes, start: int, count: int) -> list[str]:
    values: list[str] = []
    cursor = start
    for _ in range(count):
        end = blob.index(b"\0", cursor)
        values.append(blob[cursor:end].decode("utf-8"))
        cursor = end + 1
    return values


def test_ncbb_export_serializes_functions_strings_and_calls() -> None:
    payload = {
        "functions": {
            "__main__.alpha": {
                "module": "__main__",
                "name": "alpha",
                "params": ["x"],
                "code": [
                    ["LOAD_NAME", "x"],
                    ["CALL", "builtin.beta", 1],
                    ["PUSH_CONST", None],
                    ["RETURN"],
                ],
            },
            "__main__.beta": {
                "module": "__main__",
                "name": "beta",
                "params": [],
                "code": [
                    ["PUSH_CONST", "ok"],
                    ["RETURN"],
                ],
            },
        }
    }

    blob = _serialize_payload(payload)
    n_fn, n_str, n_bi, off_fn, off_bi, off_str, off_code = struct.unpack_from("<IIIIIII", blob, 8)

    assert n_fn == 2
    assert n_bi == 1
    assert off_fn == 36

    builtins = _read_pool(blob, off_bi, n_bi)
    strings = _read_pool(blob, off_str, n_str)
    assert builtins == ["builtin.beta"]
    assert "" in strings
    assert "__main__.alpha" in strings
    assert "x" in strings
    assert "__main__.beta" in strings
    assert "ok" in strings

    fn0_name_id, fn0_params, fn0_slots, fn0_code_off, fn0_code_len = struct.unpack_from("<IHHII", blob, off_fn)
    assert strings[fn0_name_id] == "__main__.alpha"
    assert fn0_params == 1
    assert fn0_slots == 1
    assert fn0_code_len > 0

    fn0_code = blob[off_code + fn0_code_off : off_code + fn0_code_off + fn0_code_len]
    # LOAD_NAME x, CALL builtin.beta, PUSH_CONST None->empty string, RETURN
    assert fn0_code[0] == 2
    assert fn0_code[3] == 21
    assert fn0_code[4:8] == struct.pack("<I", 0x80000000)
    assert fn0_code[8] == 1
    assert fn0_code[9] == 1
    assert fn0_code[10] == 2
