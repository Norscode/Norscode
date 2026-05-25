from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def _run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=str(cwd), text=True, capture_output=True, check=False)


def test_package_and_install_release_flow() -> None:
    root = Path(__file__).resolve().parents[1]
    version = "test-release-flow"
    archive = root / "release-artifacts" / f"norscode-language-{version}.tar.gz"
    checksum = Path(f"{archive}.sha256")

    with tempfile.TemporaryDirectory(prefix="norscode-release-install-") as tmpdir:
        prefix = Path(tmpdir) / "prefix"

        package = _run(["bash", "package-release.sh", version], root)
        assert package.returncode == 0, package.stderr
        assert archive.exists(), str(archive)
        assert checksum.exists(), str(checksum)

        install = _run(
            ["bash", "tools/install-release.sh", str(archive), "--prefix", str(prefix)],
            root,
        )
        assert install.returncode == 0, install.stderr
        assert (prefix / "current").exists()
        assert (prefix / "bin" / "nc").exists()

        version_check = _run([str(prefix / "bin" / "nc"), "--version"], prefix)
        assert version_check.returncode == 0, version_check.stderr
        assert "Norscode" in version_check.stdout

        doctor = _run([str(prefix / "bin" / "nc"), "--python-fallback", "doctor"], prefix)
        assert doctor.returncode == 0, doctor.stderr
        assert "Doctor OK: ja" in doctor.stdout

    archive.unlink(missing_ok=True)
    checksum.unlink(missing_ok=True)
