# REAL COMPILER V29

- fikser escape-sekvenser i selfhost-parseren for tekstliteraler
- `heltall_fra_tekst(...)` i bytecode-VM er nå trygg og returnerer `0` ved ugyldig input
- `tokeniser_uttrykk(...)` i bytecode-VM håndterer nå `!` som eget token
- selfhost-compileren normaliserer nå også kombinerte operator-tokens direkte:
  - `->`
  - `=>`
  - `<-`
  - `<->`
  - `<=>`
- liste-builtin `fjern_indeks(...)` i bytecode-VM er gjort tryggere for indekser utenfor range

Status:
- `tests/test_selfhost.no` går lenger enn i v28
- nå stopper den på en strengere parserfeil-parity i negative uttrykkstester
