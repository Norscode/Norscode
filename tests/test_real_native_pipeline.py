from __future__ import annotations

import struct

from compiler.native.pipeline import compile_source_to_native_elf, run_native_elf, verify_bootstrap_compiler
from main import registry_host


def test_real_native_pipeline_emits_machine_code_and_elf(tmp_path):
    source = tmp_path / "program.no"
    output = tmp_path / "program.elf"
    source.write_text("funksjon start() -> heltall { returner 7 + 5 }\n", encoding="utf-8")

    result = compile_source_to_native_elf(source, output)

    assert result.exit_code == 12
    assert result.machine_code.startswith(bytes.fromhex("48c7c0"))
    assert result.machine_code.endswith(bytes.fromhex("0f05"))
    assert result.elf_image.startswith(b"\x7fELF")
    assert result.elf_image[4] == 2
    assert result.elf_image[5] == 1
    assert result.executable

    entry = struct.unpack_from("<Q", result.elf_image, 24)[0]
    assert entry == result.entry_address


def test_real_native_run_builds_and_reports_host_execution_status(tmp_path):
    source = tmp_path / "program.no"
    output = tmp_path / "program.elf"
    source.write_text("funksjon start() -> heltall { returner 0 }\n", encoding="utf-8")

    result = run_native_elf(source, output)

    assert result.build.elf_image.startswith(b"\x7fELF")
    if result.ran:
        assert result.returncode == 0
    else:
        assert result.reason


def test_real_package_hosting_serves_registry_index_once(tmp_path):
    mirror = tmp_path / "registry_mirror.json"
    mirror.write_text('{"format_version":1,"packages":{"std":{"version":"0.1.0"}}}\n', encoding="utf-8")

    result = registry_host(port=0, mirror_file=str(mirror), once=True)

    assert result["ok"] is True
    assert result["count"] >= 1
    assert result["served_bytes"] == mirror.stat().st_size
    assert result["url"].endswith("/index.json")


def test_real_bootstrap_compiler_verification_has_native_artifacts():
    result = verify_bootstrap_compiler()

    assert result["ok"] is True
    assert result["elf_magic"] == "7f454c46"
    assert result["machine_code_hex"].endswith("0f05")
