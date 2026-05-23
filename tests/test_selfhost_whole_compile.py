from __future__ import annotations

import json
from pathlib import Path

from compiler.selfhost_whole_compile import WholeCompileOptions, compile_whole_norscode


def test_selfhost_whole_compile_writes_artifacts_for_parseable_root(tmp_path):
    source_root = tmp_path / "mini"
    source_root.mkdir()
    (source_root / "compiler.no").write_text(
        "funksjon start() -> heltall\n"
        "    hvis sann\n"
        "        returner 0\n"
        "    slutt\n"
        "    returner 1\n"
        "slutt\n",
        encoding="utf-8",
    )

    result = compile_whole_norscode(
        WholeCompileOptions(
            roots=(str(source_root),),
            output_dir=str(tmp_path / "out"),
        )
    )

    assert result["ok"] is True
    assert result["passed"] == 1
    assert result["failed"] == 0

    row = result["results"][0]
    assert row["ok"] is True
    assert row["bytecode"].endswith(".ncb.json")
    assert row["ast"].endswith(".shast.json")
    assert result["digest"]["algorithm"] == "sha256"
    assert result["digest"]["artifact_count"] == 2
    assert len(result["digest"]["sha256"]) == 64

    manifest = json.loads((tmp_path / "out" / "manifest.json").read_text(encoding="utf-8"))
    assert manifest["format"] == "norscode-selfhost-whole-compile-v1"
    assert manifest["digest"]["sha256"] == result["digest"]["sha256"]


def test_selfhost_whole_compile_covers_the_real_repo(tmp_path):
    result = compile_whole_norscode(
        WholeCompileOptions(
            output_dir=str(tmp_path / "whole"),
        )
    )

    assert result["ok"] is True
    assert result["failed"] == 0
    assert result["passed"] == result["total"]
    assert result["digest"]["artifact_count"] == result["total"] * 2

    compiled_files = {row["file"] for row in result["results"]}
    assert "selfhost/compiler.no" in compiled_files
    assert "compiler/compiler.no" in compiled_files

    manifest = json.loads(Path(result["manifest"]).read_text(encoding="utf-8"))
    assert manifest["ok"] is True
    assert manifest["total"] == result["total"]
