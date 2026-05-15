"""Primary Norscode CLI entrypoint.

Right now this still forwards to the Python bootstrap implementation.
The long-term goal is to replace this bridge with a self-hosted Norscode CLI.
"""

from norcode.bootstrap.python_entry import bootstrap_main



def main_cli() -> None:
    bootstrap_main()
