"""Explicit Python bootstrap entrypoint.

This module exists to make the remaining Python dependency visible and isolated.
Normal users should eventually enter Norscode through a self-hosted runtime and
CLI instead of directly through `main.py`.
"""

from main import main


def bootstrap_main() -> None:
    """Run the legacy Python bootstrap CLI."""
    main()
