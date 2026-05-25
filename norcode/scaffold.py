"""API scaffold helper for `norcode scaffold-api`."""

from __future__ import annotations

import re
from pathlib import Path


def _normalize_scaffold_project_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9]+", "_", value.strip().lower())
    cleaned = re.sub(r"_+", "_", cleaned).strip("_")
    return cleaned or "norscode_app"


def _escape_no_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def scaffold_api_project(target_dir: str, name: str | None = None, force: bool = False) -> dict:
    project_dir = Path(target_dir).expanduser().resolve()
    project_name = _normalize_scaffold_project_name(name or project_dir.name)
    pretty_name = project_name.replace("_", " ").strip().title() or "Norscode Api"
    service_name = project_name.replace("_", "-")
    json_label = _escape_no_string(project_name)
    text_label = _escape_no_string(pretty_name)

    if project_dir.exists() and any(project_dir.iterdir()) and not force:
        raise RuntimeError(f"Målmappen er ikke tom: {project_dir}")

    files: dict[str, str] = {
        "app.no": (
            "bruk std.web som web\n\n"
            "funksjon hjem(ctx: ordbok_tekst) -> ordbok_tekst {\n"
            "    web.route(\"GET /\")\n"
            f"    returner web.response_builder(200, {{\"content-type\": \"text/plain\"}}, \"Hei fra {text_label}\")\n"
            "}\n\n"
            "funksjon versjon(ctx: ordbok_tekst) -> ordbok_tekst {\n"
            "    web.route(\"GET /api/versjon\")\n"
            f"    returner web.response_builder(200, {{\"content-type\": \"application/json\"}}, \"{{\\\"navn\\\":\\\"{json_label}\\\",\\\"status\\\":\\\"klar\\\"}}\")\n"
            "}\n\n"
            "funksjon start() -> heltall {\n"
            "    returner 0\n"
            "}\n"
        ),
        "norcode.toml": (
            "[project]\n"
            f"name = \"{project_name}\"\n"
            'version = "0.1.0"\n'
            'entry = "app.no"\n\n'
            "[paths]\n"
            'source = "."\n'
            'stdlib = "std"\n'
            'build = "build"\n'
        ),
        "README.md": (
            f"# {pretty_name}\n\n"
            "Dette prosjektet ble opprettet med `norcode scaffold-api`.\n\n"
            "## Kjøring\n\n"
            "```bash\n"
            "norcode test\n"
            "norcode run app.no\n"
            "norcode serve app.no --production --host 0.0.0.0 --port 8000\n"
            "```\n\n"
            "## Struktur\n\n"
            "- `app.no` - hovedapp med web-ruter\n"
            "- `tests/` - regresjonstester for appen\n"
            "- `examples/` - små eksempelkjøringer\n"
            "- `deploy/` - service-oppsett for drift\n"
            "- `norcode.toml` - prosjektkonfigurasjon\n\n"
            "## Videre steg\n\n"
            "- legg til flere ruter i `app.no`\n"
            "- utvid testene i `tests/`\n"
            "- tilpass `deploy/norscode.service` til din serverlayout\n"
        ),
        "tests/test_app.no": (
            "bruk app\n"
            "bruk std.web som web\n\n"
            "funksjon start() -> heltall {\n"
            "    la hjem = web.handle_request(web.request_context(\"get\", \"/\", {}, {}, \"\"))\n"
            "    assert_eq(web.response_status(hjem), 200)\n"
            f"    assert_eq(web.response_body(hjem), \"Hei fra {text_label}\")\n\n"
            "    la versjon = web.handle_request(web.request_context(\"get\", \"/api/versjon\", {}, {}, \"\"))\n"
            "    assert_eq(web.response_status(versjon), 200)\n"
            f"    assert_eq(web.response_body(versjon), \"{{\\\"navn\\\":\\\"{json_label}\\\",\\\"status\\\":\\\"klar\\\"}}\")\n\n"
            "    returner 0\n"
            "}\n"
        ),
        "examples/ping.no": (
            "bruk app\n"
            "bruk std.web som web\n\n"
            "funksjon start() -> heltall {\n"
            "    la respons = web.handle_request(web.request_context(\"get\", \"/\", {}, {}, \"\"))\n"
            "    skriv(web.response_body(respons))\n"
            "    returner 0\n"
            "}\n"
        ),
        "deploy/norscode.service": (
            "[Unit]\n"
            f"Description=Norscode API service for {pretty_name}\n"
            "After=network-online.target\n"
            "Wants=network-online.target\n\n"
            "[Service]\n"
            "Type=simple\n"
            f"WorkingDirectory=/srv/{service_name}/current\n"
            f"EnvironmentFile=-/etc/{service_name}/norscode.env\n"
            "ExecStart=/srv/norscode/current/bin/nc serve app.no --production --host 0.0.0.0 --port 8000 --keep-alive --request-timeout 30 --restart-on-crash --max-restarts 3\n"
            "Restart=on-failure\n"
            "RestartSec=2\n"
            "KillSignal=SIGTERM\n"
            "TimeoutStopSec=30\n"
            "NoNewPrivileges=true\n"
            "PrivateTmp=true\n"
            "ProtectSystem=strict\n"
            "ProtectHome=true\n"
            f"ReadWritePaths=/srv/{service_name} /var/lib/{service_name} /var/log/{service_name}\n\n"
            "[Install]\n"
            "WantedBy=multi-user.target\n"
        ),
    }

    written_files: list[str] = []
    for relative_path, content in files.items():
        path = project_dir / relative_path
        if path.exists() and not force:
            raise RuntimeError(f"Filen finnes allerede: {path}")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        written_files.append(str(path.relative_to(project_dir)))

    return {
        "ok": True,
        "project_dir": str(project_dir),
        "project_name": project_name,
        "pretty_name": pretty_name,
        "files": written_files,
        "force": force,
    }
