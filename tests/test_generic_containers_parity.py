from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

from norcode.compiler_core import compile_source


SOURCE = '''funksjon summer(xs: liste<heltall>) -> heltall {
    la total = 0
    for x i xs {
        total = total + x
    }
    returner total
}

funksjon start() -> heltall {
    la tall: liste<heltall> = [1, 2, 3]
    la ord: liste<tekst> = ["a", "b", "c"]
    returner summer(tall)
}
'''


def _run_selfhost_bytecode() -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", "selfhost/tests/generic_containers_parity.no"],
        text=True,
        capture_output=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    stdout = completed.stdout.strip()
    if stdout.startswith("Return: "):
        stdout = stdout[len("Return: ") :]
    return json.loads(stdout)


def _run_python_bytecode() -> dict[str, object]:
    with tempfile.TemporaryDirectory() as tmpdir:
        source_path = Path(tmpdir) / "generic_containers.no"
        source_path.write_text(SOURCE, encoding="utf-8")
        result = compile_source(str(source_path))
        return result.bytecode


def _normalize_code(code: list[list[object]]) -> list[list[object]]:
    label_map: dict[str, str] = {}
    name_map: dict[str, str] = {}
    next_index = 0
    normalized: list[list[object]] = []

    def normalize_text(value: str) -> str:
        nonlocal next_index
        if value.startswith(("foreach_", "__iter_", "__for_")):
            if value not in name_map:
                name_map[value] = f"tmp_{next_index}"
                next_index += 1
            return name_map[value]
        return value

    for instr in code:
        op = instr[0]
        if op in {"TRY_BEGIN", "JUMP", "JUMP_IF_FALSE", "LABEL"} and len(instr) > 1 and isinstance(instr[1], str):
            label = normalize_text(instr[1])
            if label not in label_map:
                label_map[label] = f"label_{len(label_map)}"
            normalized.append([op, label_map[label]])
        elif op in {"LOAD_NAME", "STORE_NAME", "CALL"}:
            rewritten = list(instr)
            if len(rewritten) > 1 and isinstance(rewritten[1], str):
                rewritten[1] = normalize_text(rewritten[1])
            normalized.append(rewritten)
        else:
            normalized.append(list(instr))

    return normalized


def _normalize_functions(payload: dict[str, object]) -> dict[str, object]:
    functions = payload["functions"]
    normalized: dict[str, object] = {}
    for name, function in functions.items():
        fn = dict(function)
        fn["code"] = _normalize_code(fn["code"])
        normalized[name] = fn
    return normalized


def test_selfhost_generic_containers_match_python_contract() -> None:
    selfhost_bytecode = _run_selfhost_bytecode()
    python_bytecode = _run_python_bytecode()

    assert selfhost_bytecode["format"] == python_bytecode["format"]
    assert selfhost_bytecode["entry"] == python_bytecode["entry"]
    assert _normalize_functions(selfhost_bytecode) == _normalize_functions(python_bytecode)
    assert selfhost_bytecode["route_handlers"] == python_bytecode["route_handlers"]
    assert selfhost_bytecode["startup_hooks"] == python_bytecode["startup_hooks"]
