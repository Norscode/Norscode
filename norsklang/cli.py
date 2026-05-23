import sys

from norcode.cli import main_cli as _norcode_main_cli


def main_cli():
    print("Merk: 'norsklang' er legacy alias. Bruk 'norcode' videre.", file=sys.stderr)
    _norcode_main_cli()
