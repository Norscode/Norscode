NORSCODE ?= ./bin/nc

.PHONY: install test run check build feature-check maturity ci version commands release-package

# Installer native bootstrap-kompilator (Python-fri etter første bygging)
install:
	$(NORSCODE) run tools/build-bootstrap-binary.no

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

maturity:
	$(NORSCODE) maintenance maturity

version:
	$(NORSCODE) --version

commands:
	$(NORSCODE) commands

# CI: bruk selfhost-bootstrap-gate direkte i stedet for --check-names (Python)
ci:
	$(NORSCODE) ci --bootstrap-lane

release-package:
	NORSCODE_ENABLE_EXEC_PROSESS=1 $(NORSCODE) run tools/package_release.no
