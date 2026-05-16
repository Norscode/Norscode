import sys

from norcode.__main__ import main_cli as _norcode_main_cli


if __name__ == "__main__":
    print("Merk: 'python -m norsklang' er legacy alias. Bruk 'python -m norcode'.", file=sys.stderr)
    _norcode_main_cli()
