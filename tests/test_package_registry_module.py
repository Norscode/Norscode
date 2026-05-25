from __future__ import annotations

from pathlib import Path

from norcode.package_registry import generate_lockfile, registry_sign, verify_lockfile


def _seed_project(root: Path) -> None:
    (root / "pkg").mkdir()
    (root / "pkg" / "norcode.toml").write_text('[project]\nname = "pkg"\n', encoding="utf-8")
    (root / "packages").mkdir()
    (root / "packages" / "registry.toml").write_text('[packages]\ncore = { description = "Core", path = "./pkg" }\n', encoding="utf-8")
    (root / "norcode.toml").write_text(
        '[project]\nname = "demo"\nversion = "0.1.0"\nentry = "main.no"\n\n[dependencies]\ncore = "./pkg"\n',
        encoding="utf-8",
    )
    (root / "main.no").write_text('print("ok")\n', encoding="utf-8")


def test_registry_sign_and_lockfile_flow(tmp_path: Path, monkeypatch) -> None:
    _seed_project(tmp_path)
    monkeypatch.chdir(tmp_path)

    signed = registry_sign(write_config=True)
    assert signed["written_to_config"] is True
    assert signed["sha256"]
    assert 'trusted_registry_sha256' in (tmp_path / "norcode.toml").read_text(encoding="utf-8")

    lock_path, ok, status = generate_lockfile(check_only=False)
    assert ok is True
    assert status == "skrevet"
    assert lock_path.exists()

    verify_path, verify_ok, results = verify_lockfile()
    assert verify_path.exists()
    assert verify_ok is True
    assert any(row["name"] == "core" and row["status"] == "ok" for row in results)
