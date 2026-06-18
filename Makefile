NORSCODE ?= ./bin/nc

.PHONY: install test run check build feature-check ci version commands release-package

# Installer native bootstrap-kompilator (Python-fri etter første bygging)
install:
	bash tools/build-bootstrap-binary.sh

# ─── Vanlige daglige kommandoer (native, ingen Python) ──────────────────────

test:
	$(NORSCODE) test

run:
	$(NORSCODE) run app.no

check:
	$(NORSCODE) check app.no

build:
	$(NORSCODE) build app.no app.elf

feature-check:
	$(NORSCODE) feature-check app.no

version:
	$(NORSCODE) --version

commands:
	$(NORSCODE) commands

# CI: bruk selfhost-bootstrap-gate direkte i stedet for --check-names (Python)
ci:
	$(NORSCODE) ci --bootstrap-lane

release-package:
	./package-release.sh
