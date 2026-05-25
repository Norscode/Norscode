from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

from norcode.compiler_core import compile_source


SOURCE = """
bruk std.web som web

funksjon status() -> tekst { web.route("GET /status") returner "ok" }

funksjon start() -> tekst { returner status() }
""".strip()


def _run_selfhost_bytecode() -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", "selfhost/tests/web_parity.no"],
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
        source_path = Path(tmpdir) / "web.no"
        source_path.write_text(SOURCE, encoding="utf-8")
        result = compile_source(str(source_path))
        return result.bytecode


def test_selfhost_web_metadata_matches_python_contract() -> None:
    selfhost_bytecode = _run_selfhost_bytecode()
    python_bytecode = _run_python_bytecode()

    assert selfhost_bytecode["format"] == python_bytecode["format"]
    assert selfhost_bytecode["entry"] == python_bytecode["entry"]
    assert selfhost_bytecode["route_handlers"] == python_bytecode["route_handlers"]
    assert selfhost_bytecode["dependency_providers"] == python_bytecode["dependency_providers"]
    assert selfhost_bytecode["guard_providers"] == python_bytecode["guard_providers"]
    assert selfhost_bytecode["request_middlewares"] == python_bytecode["request_middlewares"]
    assert selfhost_bytecode["response_middlewares"] == python_bytecode["response_middlewares"]
    assert selfhost_bytecode["error_middlewares"] == python_bytecode["error_middlewares"]
    assert selfhost_bytecode["startup_hooks"] == python_bytecode["startup_hooks"]
    assert selfhost_bytecode["shutdown_hooks"] == python_bytecode["shutdown_hooks"]
