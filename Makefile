PYTHON ?= python3
PIP ?= $(PYTHON) -m pip
NORSCODE ?= ./bin/nc

.PHONY: install install-dev test run check build ci release-package

install:
	$(PIP) install -e .

install-dev:
	$(PIP) install -e .
	$(PIP) install -r requirements-dev.txt

test:
	$(NORSCODE) test

run:
	$(NORSCODE) run app.no

check:
	$(NORSCODE) check app.no

build:
	$(NORSCODE) build app.no

ci:
	$(NORSCODE) ci --check-names

release-package:
	./package-release.sh
