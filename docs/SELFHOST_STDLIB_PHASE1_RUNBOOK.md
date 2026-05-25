# Selfhost Stdlib Phase 1 Runbook

Mål:
Gjere den stdlib-flaten som compilerarbeid er avhengig av, synleg og testbar utan skjulte Python-hjelparar.

Kanoniske moduler i denne runden:

- [`std/tekst.no`](/Users/jansteinar/Projects/Norscode/std/tekst.no)
- [`std/liste.no`](/Users/jansteinar/Projects/Norscode/std/liste.no)
- [`std/ordbok.no`](/Users/jansteinar/Projects/Norscode/std/ordbok.no)
- [`std/fil.no`](/Users/jansteinar/Projects/Norscode/std/fil.no)
- [`std/path.no`](/Users/jansteinar/Projects/Norscode/std/path.no)
- [`std/env.no`](/Users/jansteinar/Projects/Norscode/std/env.no)
- [`std/json.no`](/Users/jansteinar/Projects/Norscode/std/json.no)
- [`std/feil.no`](/Users/jansteinar/Projects/Norscode/std/feil.no)
- [`std/cli.no`](/Users/jansteinar/Projects/Norscode/std/cli.no)

Kanonisk compilerbruk:

- [`selfhost/bootstrap_compiler/compiler_entrypoint.no`](/Users/jansteinar/Projects/Norscode/selfhost/bootstrap_compiler/compiler_entrypoint.no)
- [`selfhost/tests/stdlib_compiler_tests.no`](/Users/jansteinar/Projects/Norscode/selfhost/tests/stdlib_compiler_tests.no)

Kjøring:

- `./bin/nc selfhost-stdlib-suite`
- `python3 -m norcode selfhost-stdlib-suite`
- `./bin/nc run selfhost/tests/stdlib_compiler_tests.no`

Forventet resultat:

- `Selfhost stdlib suite: 1/1 OK`
- `stdlib/compiler tester` går grønt
- CLI-parsing i bootstrap compiler bruker `std.cli`

