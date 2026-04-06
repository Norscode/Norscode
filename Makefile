PYTHON ?= python3
PIP ?= $(PYTHON) -m pip

.PHONY: install install-dev test run check build ci

install:
	$(PIP) install -e .

install-dev:
	$(PIP) install -e .
	$(PIP) install -r requirements-dev.txt

test:
	$(PYTHON) main.py test

run:
	$(PYTHON) main.py run app.no

check:
	$(PYTHON) main.py check app.no

build:
	$(PYTHON) main.py build app.no

ci:
	$(PYTHON) main.py ci --check-names
