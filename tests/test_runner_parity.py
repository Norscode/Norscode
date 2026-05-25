from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

from norcode.compiler_core import compile_source


SOURCE = 'test "summerer" { assert_eq(1 + 2, 3) }\n\nfunksjon start() -> heltall { returner 0 }\n'


def _run_selfhost_bytecode() -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", "selfhost/tests/test_runner_parity.no"],
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
        source_path = Path(tmpdir) / "runner.no"
        source_path.write_text(SOURCE, encoding="utf-8")
        result = compile_source(str(source_path))
        return result.bytecode


def test_selfhost_test_runner_metadata_matches_python_contract() -> None:
    selfhost_bytecode = _run_selfhost_bytecode()
    python_bytecode = _run_python_bytecode()

    assert selfhost_bytecode["format"] == python_bytecode["format"]
    assert selfhost_bytecode["entry"] == python_bytecode["entry"]
    assert selfhost_bytecode["tests"] == python_bytecode["tests"]
    assert selfhost_bytecode["functions"] == python_bytecode["functions"]
    assert selfhost_bytecode["route_handlers"] == python_bytecode["route_handlers"]
    assert selfhost_bytecode["startup_hooks"] == python_bytecode["startup_hooks"]
