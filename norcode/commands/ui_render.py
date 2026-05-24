"""Render the Native UI syntax to HTML using a Norscode runner."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

from norcode.commands.base import CommandModule
REPO_ROOT = Path(__file__).resolve().parents[2]
NATIVE_UI_RENDERER = REPO_ROOT / "std" / "nativeui.no"


def render_native_ui(file: str, output: str | None = None, title: str | None = None) -> int:
    keys = ("NORSCODE_UI_INPUT", "NORSCODE_UI_OUTPUT", "NORSCODE_UI_TITLE")
    previous = {key: os.environ.get(key) for key in keys}
    try:
        os.environ["NORSCODE_UI_INPUT"] = str(Path(file).expanduser().resolve())
        os.environ["NORSCODE_UI_OUTPUT"] = str(Path(output).expanduser().resolve()) if output else ""
        os.environ["NORSCODE_UI_TITLE"] = title or ""
        completed = subprocess.run(
            [str(REPO_ROOT / "bin" / "nc"), "run", str(NATIVE_UI_RENDERER)],
            check=True,
            text=True,
            capture_output=True,
        )
        stdout = completed.stdout.replace("Return: 0\r\n", "").replace("Return: 0\n", "")
        if stdout:
            print(stdout, end="")
        return int(completed.returncode)
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value


def register_arguments(parser) -> None:
    parser.add_argument("file", help="UI-kildefil med innrykket `side:`/`kort:`-syntaks")
    parser.add_argument("--output", "-o", help="Skriv HTML til fil i stedet for stdout")
    parser.add_argument("--title", default=None, help="Overstyr sidetittel i HTML-dokumentet")


def run(args) -> int:
    return render_native_ui(args.file, output=args.output, title=args.title)


UI_RENDER_COMMAND = CommandModule(
    name="ui-render",
    help="Render Native UI-syntax til HTML",
    register_arguments=register_arguments,
    run=run,
    experimental=True,
)
