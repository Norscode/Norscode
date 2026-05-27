"""Explicit Python bootstrap entrypoint.

This module keeps the remaining Python bootstrap path visible and isolated.
Normal users should enter Norscode through the modular CLI; this entrypoint is
here as the compatibility bridge until the self-hosted runtime is complete.
"""

from norcode.legacy_main import main


def bootstrap_main() -> None:
    """Run the compatibility bootstrap CLI."""
    main()
