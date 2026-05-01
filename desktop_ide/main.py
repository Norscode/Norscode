#!/usr/bin/env python3
"""Norscode Studio desktop shell.

Lokal studio-app med Tkinter-grensesnitt:
- Kjøring av Norscode CLI
- AI-assistanse med Codex/Gemini
"""

from __future__ import annotations

import argparse
import json
import os
import queue
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Callable
from typing import Optional

try:
    import tkinter as tk
    from tkinter import scrolledtext
    from tkinter import ttk
except Exception as exc:  # noqa: BLE001
    tk = None
    scrolledtext = None
    ttk = None
    _TK_IMPORT_ERROR = f"Tkinter kan ikke importeres: {exc}"
else:
    _TK_IMPORT_ERROR = ""


def find_project_root() -> Path:
    seed_paths = [Path(__file__).resolve().parent, Path.cwd().resolve()]
    extra = os.environ.get("NORSCODE_ROOT")
    if extra:
        seed_paths.insert(0, Path(extra).resolve())

    meipass = Path(getattr(sys, "_MEIPASS", "")) if hasattr(sys, "_MEIPASS") else None
    if meipass and meipass.exists():
        seed_paths.insert(0, meipass)

    for seed in seed_paths:
        for candidate in [seed] + list(seed.parents):
            if (candidate / "projects" / "language" / "desktop_ide" / "main.py").is_file():
                return candidate
            if (candidate / "projects" / "language" / "desktop_ide").is_dir():
                return candidate

    raise SystemExit(
        "Fant ikke prosjektroten. Kjør fra språkroten, eller sett NORSCODE_ROOT."
    )


PROJECT_ROOT = find_project_root()
WORKSPACE_ROOT = PROJECT_ROOT / "projects" / "language"
if not WORKSPACE_ROOT.exists():
    WORKSPACE_ROOT = PROJECT_ROOT


def run_norscode(mode: str, source: str) -> tuple[int, str, str]:
    mode = (mode or "run").strip() or "run"
    with tempfile.NamedTemporaryFile("w", suffix=".no", prefix="norscode-studio-", delete=False) as f:
        f.write(source)
        path = Path(f.name)

    cli = os.environ.get("NORSCODE_CLI", "norscode").strip() or "norscode"
    cmd = [cli, mode, str(path)]

    try:
        result = subprocess.run(
            cmd,
            cwd=str(WORKSPACE_ROOT),
            capture_output=True,
            text=True,
            check=False,
            timeout=30,
        )
        stdout = result.stdout or ""
        stderr = result.stderr or ""
        if result.returncode == 0:
            return 200, stdout, stderr
        return 500, stdout, stderr
    except FileNotFoundError:
        msg = f"Fant ikke CLI-kommandoen '{cli}'. Sett NORSCODE_CLI riktig."
        return 404, "", msg
    except subprocess.TimeoutExpired:
        return 408, "", "Kjøring tok for lang tid (>30 sekunder)."
    except Exception as exc:  # noqa: BLE001
        return 500, "", f"Feil ved kjøring: {exc}"
    finally:
        try:
            path.unlink()
        except OSError:
            pass


def _parse_engine_output(stdout: str) -> tuple[bool, Optional[dict[str, object]]]:
    for line in reversed((stdout or "").splitlines()):
        line = line.strip()
        if not line.startswith("{") or not line.endswith("}"):
            continue
        try:
            data = json.loads(line)
            if isinstance(data, dict):
                return True, data
        except json.JSONDecodeError:
            continue
    return False, None


def _extract_provider_text(provider: str, raw: str) -> str:
    if not raw:
        return ""
    try:
        payload = json.loads(raw)
    except (TypeError, json.JSONDecodeError):
        return raw.strip()

    provider_lc = provider.strip().lower()
    if provider_lc == "codex":
        choices = payload.get("choices")
        if isinstance(choices, list) and choices:
            first = choices[0]
            if isinstance(first, dict):
                message = first.get("message")
                if isinstance(message, dict):
                    content = message.get("content")
                    if isinstance(content, str) and content.strip():
                        return content.strip()
    if provider_lc == "gemini":
        candidates = payload.get("candidates")
        if isinstance(candidates, list) and candidates:
            first_candidate = candidates[0]
            if isinstance(first_candidate, dict):
                candidate_content = first_candidate.get("content")
                if isinstance(candidate_content, dict):
                    parts = candidate_content.get("parts")
                    if isinstance(parts, list) and parts:
                        first_part = parts[0]
                        if isinstance(first_part, dict):
                            text = first_part.get("text")
                            if isinstance(text, str) and text.strip():
                                return text.strip()
    if isinstance(payload, dict):
        content = payload.get("text")
        if isinstance(content, str) and content.strip():
            return content.strip()
    return raw.strip()


def run_studio_engine(command: str, source: str, prompt: str, provider: str = "codex") -> tuple[bool, str, str]:
    workspace_studio = Path(__file__).resolve().parent / "studio_engine.no"

    if not workspace_studio.exists():
        return False, "Fant ikke studio_engine.no", ""

    with tempfile.NamedTemporaryFile("w", suffix=".no", prefix="norscode-studio-source-", delete=False) as src_file:
        src_file.write(source)
        source_path = Path(src_file.name)

    with tempfile.NamedTemporaryFile("w", suffix=".txt", prefix="norscode-studio-prompt-", delete=False) as prompt_file:
        prompt_file.write(prompt)
        prompt_path = Path(prompt_file.name)

    cli = os.environ.get("NORSCODE_CLI", "norscode").strip() or "norscode"
    cmd = [cli, "run", str(workspace_studio)]

    env = {
        **os.environ,
        "NORSCODE_STUDIO_CMD": command,
        "NORSCODE_STUDIO_PROVIDER": provider,
        "NORSCODE_STUDIO_SOURCE_PATH": str(source_path),
        "NORSCODE_STUDIO_PROMPT": str(prompt_path),
    }

    try:
        result = subprocess.run(
            cmd,
            cwd=str(WORKSPACE_ROOT),
            env=env,
            capture_output=True,
            text=True,
            check=False,
            timeout=60,
        )
        parsed, payload = _parse_engine_output(result.stdout or "")
        if not parsed or payload is None:
            stderr = (result.stderr or "").strip()
            stdout = (result.stdout or "").strip()
            fallback = stderr or stdout or "Ingen gyldig respons fra studio-engine."
            return False, fallback, fallback

        ok = str(payload.get("ok", "usann")).lower() == "sann"
        engine_text = str(payload.get("text", "")).strip()
        raw = str(payload.get("raw", ""))

        if not ok:
            return False, engine_text, raw

        provider_reply = _extract_provider_text(payload.get("provider", provider) or provider, raw)
        if provider_reply:
            return True, provider_reply, raw
        if engine_text:
            return True, engine_text, raw
        return False, "Tom svar fra studio-engine.", raw

    except FileNotFoundError:
        return False, f"Fant ikke CLI-kommandoen '{cli}'. Sett NORSCODE_CLI riktig.", ""
    except subprocess.TimeoutExpired:
        return False, "Studio-engine tok for lang tid (>60 sekunder).", ""
    except Exception as exc:  # noqa: BLE001
        return False, f"Feil ved studio-engine: {exc}", ""
    finally:
        try:
            source_path.unlink()
        except OSError:
            pass
        try:
            prompt_path.unlink()
        except OSError:
            pass


def can_use_tk() -> tuple[bool, str]:
    if tk is None or scrolledtext is None or ttk is None:
        return False, _TK_IMPORT_ERROR or "Tkinter-modulene mangler i dette miljøet."
    try:
        root = tk.Tk()
        root.withdraw()
        root.update_idletasks()
        root.destroy()
        return True, ""
    except Exception as exc:  # noqa: BLE001
        return False, f"Tkinter kan ikke initialiseres: {exc}"


class StudioUI:
    def __init__(self, startup_hint: str | None = None) -> None:
        self.startup_hint = startup_hint
        self.root = tk.Tk()
        self.root.title("Norscode Studio")
        self.root.geometry("1220x780")
        self.root.minsize(1100, 650)

        self.messages: "queue.Queue[tuple[str, str]]" = queue.Queue()
        self.current_thread: threading.Thread | None = None
        self._setup_ui()

        self._process_messages()

    def _setup_ui(self) -> None:
        outer = ttk.Frame(self.root, padding=12)
        outer.pack(fill="both", expand=True)

        top = ttk.Frame(outer)
        top.pack(fill="x", pady=(0, 10))
        ttk.Label(top, text="Norscode Studio", font=("Arial", 18, "bold")).pack(side="left")
        startup_text = "Klar"
        if self.startup_hint:
            startup_text = f"{startup_text} • {self.startup_hint}"
        self.status = ttk.Label(top, text=startup_text, foreground="#2f4f4f")
        self.status.pack(side="right")

        container = ttk.PanedWindow(outer, orient="horizontal")
        container.pack(fill="both", expand=True)

        left = ttk.Frame(container)
        right = ttk.Frame(container)
        container.add(left, weight=2)
        container.add(right, weight=1)

        self._build_editor(left)
        self._build_controls(left)
        self._build_output_tabs(right)

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def _build_editor(self, parent: ttk.Frame) -> None:
        frame = ttk.LabelFrame(parent, text="Kildekode", padding=8)
        frame.pack(fill="both", expand=True)
        self.editor = scrolledtext.ScrolledText(frame, font=("Menlo", 12), wrap="word", undo=True)
        self.editor.pack(fill="both", expand=True)
        self.editor.insert("1.0", "skriv(\"Hei fra Norscode Studio\")\n")

    def _build_controls(self, parent: ttk.Frame) -> None:
        frame = ttk.Frame(parent)
        frame.pack(fill="x", pady=8)

        self.file_name = tk.StringVar(value="main.no")
        self.mode = tk.StringVar(value="run")
        self.provider = tk.StringVar(value="codex")

        top_row = ttk.Frame(frame)
        top_row.pack(fill="x")
        ttk.Label(top_row, text="Fil:").pack(side="left")
        ttk.Entry(top_row, textvariable=self.file_name, width=26).pack(side="left", padx=5)
        ttk.Label(top_row, text="Modus:").pack(side="left", padx=(12, 0))
        ttk.OptionMenu(top_row, self.mode, "run", "run", "test", "fmt", "lint").pack(side="left")
        ttk.Label(top_row, text="AI-assistent:").pack(side="left", padx=(12, 0))
        ttk.OptionMenu(top_row, self.provider, "codex", "codex", "gemini").pack(side="left")

        button_row = ttk.Frame(frame)
        button_row.pack(fill="x", pady=(6, 0))
        ttk.Button(button_row, text="Kjør kode", command=self.on_run).pack(side="left", padx=(0, 4))
        ttk.Button(button_row, text="Spør AI", command=self.on_ask_ai).pack(side="left", padx=(0, 4))
        ttk.Button(button_row, text="Refaktorer", command=self.on_refactor).pack(side="left", padx=(0, 4))
        ttk.Button(button_row, text="Tøm konsoll", command=self._clear_output).pack(side="left")

        prompt_row = ttk.Frame(frame)
        prompt_row.pack(fill="x", pady=(8, 0))
        ttk.Label(prompt_row, text="AI prompt:").pack(side="left")
        self.prompt = tk.StringVar(
            value="Forklar koden min, gi konkrete forbedringer og eventuelle feil."
        )
        ttk.Entry(prompt_row, textvariable=self.prompt).pack(side="left", fill="x", expand=True, padx=6)

    def _build_output_tabs(self, parent: ttk.Frame) -> None:
        tabs = ttk.Notebook(parent)
        tabs.pack(fill="both", expand=True)

        run_frame = ttk.Frame(tabs)
        ai_frame = ttk.Frame(tabs)
        history_frame = ttk.Frame(tabs)

        tabs.add(run_frame, text="Kjøring")
        tabs.add(ai_frame, text="AI-logg")
        tabs.add(history_frame, text="Historikk")

        self.run_output = scrolledtext.ScrolledText(run_frame, font=("Menlo", 11), wrap="word")
        self.run_output.pack(fill="both", expand=True)
        self.run_output.insert("1.0", "Konsoll for kjøring")

        self.ai_output = scrolledtext.ScrolledText(ai_frame, font=("Menlo", 11), wrap="word")
        self.ai_output.pack(fill="both", expand=True)
        self.ai_output.insert("1.0", "AI-tilbakemeldinger vises her")

        self.history_output = scrolledtext.ScrolledText(history_frame, font=("Menlo", 11), wrap="word")
        self.history_output.pack(fill="both", expand=True)
        self.history_output.insert("1.0", "Historikk:")

    def on_run(self) -> None:
        self._run_async("run", self._do_run_code)

    def on_ask_ai(self) -> None:
        self._run_async("ask", self._do_ask_ai)

    def on_refactor(self) -> None:
        self.prompt.set("Refaktor koden for lesbarhet og enklere vedlikehold.")
        self._run_async("refactor", self._do_refactor)

    def _run_async(self, tag: str, action: Callable[[], str]) -> None:
        if self.current_thread and self.current_thread.is_alive():
            self._append_run("En operasjon kjører allerede. Vent et øyeblikk.")
            self._append_history("Venter på at forrige operasjon avsluttes.")
            return
        self._set_status(f"Kjører: {tag} ...")
        self.current_thread = threading.Thread(target=self._worker, args=(tag, action), daemon=True)
        self.current_thread.start()

    def _worker(self, tag: str, action: Callable[[], str]) -> None:
        try:
            output = action()
            self.messages.put((f"{tag}_ok", output))
        except Exception as exc:  # noqa: BLE001
            self.messages.put((f"{tag}_err", str(exc)))

    def _do_run_code(self) -> str:
        mode = self.mode.get().strip() or "run"
        source = self.editor.get("1.0", "end-1c")
        status_code, stdout, stderr = run_norscode(mode, source)
        if status_code == 200:
            return (
                f"[{time.strftime('%H:%M:%S')}] OK {mode} ({self.file_name.get()})\n"
                f"--- STDOUT ---\n{stdout}\n--- STDERR ---\n{stderr}"
            )
        return (
            f"[{time.strftime('%H:%M:%S')}] FEIL {status_code} ved {mode} ({self.file_name.get()})\n"
            f"--- STDOUT ---\n{stdout}\n--- STDERR ---\n{stderr}"
        )

    def _do_ask_ai(self) -> str:
        provider = self.provider.get().strip().lower() or "codex"
        source = self.editor.get("1.0", "end-1c")
        prompt = self.prompt.get().strip() or "Gi en kort vurdering av koden."
        full_prompt = f"{prompt}\n\nKildekode:\n```\n{source}\n```"
        success, text, raw = run_studio_engine("ask", source, full_prompt, provider)
        if not success:
            return f"[{time.strftime('%H:%M:%S')}] {provider.upper()} FEIL:\n{text}"
        return f"[{time.strftime('%H:%M:%S')}] {provider.upper()} svar:\n{text}"

    def _do_refactor(self) -> str:
        provider = self.provider.get().strip().lower() or "codex"
        source = self.editor.get("1.0", "end-1c")
        prompt = (
            "Refaktor følgende kode for bedre struktur og lesbarhet. "
            "Behold funksjonalitet og returner kun den forbedrede koden."
        )
        full_prompt = f"{prompt}\n\n```\n{source}\n```"
        success, text, raw = run_studio_engine("refactor", source, full_prompt, provider)
        if not success:
            return f"[{time.strftime('%H:%M:%S')}] {provider.upper()} FEIL:\n{text}"

        replacement = text.strip()
        if not replacement:
            replacement = _extract_provider_text(provider, raw).strip()

        if not replacement:
            return f"[{time.strftime('%H:%M:%S')}] {provider.upper()} returnerte tom tekst."

        self.messages.put(("replace", replacement))
        return f"[{time.strftime('%H:%M:%S')}] {provider.upper()} foreslo oppdatert kode. Innsatt i editor."

    def _append_run(self, text: str) -> None:
        self.run_output.insert("end", text + "\n")
        self.run_output.see("end")

    def _append_ai(self, text: str) -> None:
        self.ai_output.insert("end", text + "\n")
        self.ai_output.see("end")

    def _append_history(self, text: str) -> None:
        self.history_output.insert("end", text + "\n")
        self.history_output.see("end")

    def _clear_output(self) -> None:
        self.run_output.delete("1.0", "end")
        self.ai_output.delete("1.0", "end")
        self._append_run("Konsoll tømt.")

    def _set_status(self, text: str) -> None:
        self.status.configure(text=text)
        self.status.update_idletasks()

    def _process_messages(self) -> None:
        while not self.messages.empty():
            tag, text = self.messages.get()
            if tag == "run_ok":
                self._append_run(text)
                self._append_history(text)
            elif tag == "ask_ok":
                self._append_ai(text)
                self._append_history(text)
            elif tag == "refactor_ok":
                self._append_ai(text)
                self._append_history(text)
            elif tag.endswith("_err"):
                self._append_run(text)
                self._append_ai(text)
                self._append_history(f"FEIL: {text}")
            elif tag == "replace":
                self.editor.delete("1.0", "end")
                self.editor.insert("1.0", text)

            self._set_status("Klar")
            self.messages.task_done()

        self.root.after(100, self._process_messages)

    def on_close(self) -> None:
        self.root.destroy()

    def run(self) -> int:
        self.root.mainloop()
        return 0


def run_text_mode(reason: str = "") -> int:
    """Kjør en enkel tekstbasert versjon for miljøer uten GUI."""
    print("Norscode Studio kan ikke starte GUI i dette miljøet.")
    if reason:
        print(f"Grunn: {reason}")
    print("Legg inn kode og bruk: python3 desktop_ide/main.py --no-gui (text mode).")
    print("For AI-integrasjon, sett CODEX_API_KEY / GEMINI_API_KEY før kjøring.")
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(description="Start Norscode Studio.")
    parser.add_argument("--no-gui", action="store_true", help="Kjør tekstmodus i terminal")
    args = parser.parse_args()

    if args.no_gui:
        return run_text_mode("Bruker aktiverte --no-gui.")

    can_use, reason = can_use_tk()
    if not can_use:
        return run_text_mode(reason)

    app = StudioUI(startup_hint="GUI-check: OK / Display OK")
    return app.run()


if __name__ == "__main__":
    raise SystemExit(main())
