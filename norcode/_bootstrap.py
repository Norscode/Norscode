from __future__ import annotations

import importlib
import sys
from pathlib import Path
from typing import Callable


def _project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def ensure_project_root_on_syspath() -> Path:
    root = _project_root()
    root_str = str(root)
    if root_str not in sys.path:
        sys.path.insert(0, root_str)
    return root


def load_main_callable() -> Callable[[], None]:
    ensure_project_root_on_syspath()
    try:
        module = importlib.import_module("main")
    except Exception as exc:  # pragma: no cover - only used at runtime
        raise RuntimeError(
            "Kunne ikke laste Norscode CLI. "
            "Sjekk at prosjektet er installert riktig med 'python3 -m pip install -e .'."
        ) from exc

    main_func = getattr(module, "main", None)
    if not callable(main_func):  # pragma: no cover - runtime safety
        raise RuntimeError("Fant ikke en gyldig 'main()' i prosjektets main.py.")
    return main_func


def run_main() -> None:
    main_func = load_main_callable()
    main_func()
