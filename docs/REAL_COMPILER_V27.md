# REAL COMPILER V27

V27 forbedrer selfhost-kjeden på to viktige punkter:

1. Kommentar-stripping respekterer nå tekststrenger.
2. Module-qualified builtin-kall kan brukes i bytecode-VM-en.

Dette gjør at `test_selfhost.no` kommer lenger i kjeden, selv om hele testen ennå ikke er grønn.
