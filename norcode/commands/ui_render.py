"""Render the Native UI syntax to HTML using a Norscode runner."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from norcode.commands.base import CommandModule
REPO_ROOT = Path(__file__).resolve().parents[2]
NATIVE_UI_RENDERER = REPO_ROOT / "std" / "nativeui.no"


def render_native_ui(file: str, output: str | None = None, title: str | None = None) -> int:
    keys = ("NORSCODE_UI_INPUT", "NORSCODE_UI_OUTPUT", "NORSCODE_UI_TITLE", "NORSCODE_SUPPRESS_RETURN")
    previous = {key: os.environ.get(key) for key in keys}
    try:
        os.environ["NORSCODE_UI_INPUT"] = str(Path(file).expanduser().resolve())
        os.environ["NORSCODE_UI_OUTPUT"] = str(Path(output).expanduser().resolve()) if output else ""
        os.environ["NORSCODE_UI_TITLE"] = title or ""
        os.environ["NORSCODE_SUPPRESS_RETURN"] = "1"
        completed = subprocess.run(
            [str(REPO_ROOT / "bin" / "nc"), "run", str(NATIVE_UI_RENDERER)],
            check=True,
            text=True,
            capture_output=True,
        )
        if completed.stdout:
            print(completed.stdout, end="")
        return int(completed.returncode)
    except subprocess.CalledProcessError as exc:
        if exc.stdout:
            print(exc.stdout, end="")
        if exc.stderr:
            sys.stderr.write(exc.stderr)
            if not exc.stderr.endswith("\n"):
                sys.stderr.write("\n")
        return int(exc.returncode)
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
