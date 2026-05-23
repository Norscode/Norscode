NORSCODE ?= ./bin/nc

.PHONY: install install-dev install-python test run check build ci version commands release-package

# Installer native bootstrap-kompilator (Python-fri etter første bygging)
install:
	bash tools/build-bootstrap-binary.sh

# Kun for utviklere som trenger Python-sporet (f.eks. for å jobbe på kompilatoren selv)
install-python:
	LEGACY_PYTHON=1 bash scripts/dev-setup.sh

# Bakoverkompatibelt alias
install-dev:
	LEGACY_PYTHON=1 bash scripts/dev-setup.sh

# ─── Vanlige daglige kommandoer (native, ingen Python) ──────────────────────

test:
	$(NORSCODE) test

run:
	$(NORSCODE) run app.no

check:
	$(NORSCODE) check app.no

build:
	$(NORSCODE) build app.no app.elf

version:
	$(NORSCODE) --version

commands:
	$(NORSCODE) commands

# CI: bruk selfhost-bootstrap-gate direkte i stedet for --check-names (Python)
ci:
	$(NORSCODE) ci --bootstrap-lane

release-package:
	./package-release.sh
