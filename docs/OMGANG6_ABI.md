# Omgang 6 · M2/M4/M6/M9 i AOT

**Innsikt:** tre av fire var ALLEREIE dekt av native — berre M6 (typa unnatak)
trong nytt arbeid.

- **M4 `ingenting`-type** — native handterar `LOAD_NAME ingenting`/`PUSH_CONST
  ingenting` som boksa int 0 (som tolken). ✓ Ingen jobb.
- **M9 enum + grensesnitt** — enum-tilgang lowrar til `PUSH_CONST <heiltal>` på
  compile-tid (`ir_to_bytecode` slår opp `komp["enums"]`); grensesnitt-kontrakt er
  ein compile-tid semantisk sjekk (struktur har alle metodane). Begge er ferdige
  før native køyrer. ✓ Ingen jobb.
- **M2 fangbare feil + stack traces** — try/catch (TRY_BEGIN/END/THROW/
  LOAD_EXCEPTION) er alt i native; `std.trace` er eit reint Norscode-bibliotek som
  køyrer på vanlege opcodar. ✓ Ingen ny opcode.

## M6 — typa unnatak (`fang (e: Type)`) *(GJORT)*

Kompilatoren gjev `TRY_BEGIN catch_label catch_type`. Tolken lagrar `catch_type` i
handleren og THROW matchar exc-typen mot han (`vm_handler_match`/
`vm_exception_type`): map-`"type"`-felt, streng-`"Type:"`-prefiks, eller `""`/`*`/
`alle` = fang-alt; ikkje-match → propager til ytre handler. Native gjorde FØR
alltid unwind til inste handler (feil for typa catch som skal propagere).

**Native design:**
- **Handler-record[72]** (før ubrukt) held `catch_type`-strengpeikaren; `0` =
  fang-alt. TRY_BEGIN byggjer strengen i x11 FØR record-bygginga (str_const
  klobbar x9/x10 = record-peikaren), lagrar etter.
- **Nye globalar** `__exc_base__` (handler-stakk-basis, uncaught-deteksjon) +
  `__exc_type__` (rekna exc-type). Entry set `__exc_base__ = __exc_top__`-basis.
- **`__throw_dispatch__`** (delt rutine): reknar exc-type ÉIN gong (map_get på
  `"type"` / str_find+slice på `":"` / `"tekst"`), går så handler-kjeda frå toppen
  og str_eq mot kvar `catch_type`; fyrste match (eller `0`=fang-alt) → set
  `__exc_top__ = handler+80`, gjenopprett sp/x19–x24/x26, `br` til catch. Ingen
  match → `exit(1)`. Delegerer tung strengjobb til prova hjelpe-emittarar
  (map_get/str_find/slice/str_eq); berre glue + kjede-loop er ny handkoda asm.
- **THROW/feil/gap_stub** lagrar `__exc_val__` og `bl __throw_dispatch__` (erstattar
  inline-unwind). Appenda i imaget når bl-referert (compile_program + ncval_link;
  batch les plan-globals for __exc-indeksane).

**Verifikasjon (VIKTIG):** strukturell codegen-sjekk stadfestar berre at koden
KOMPILERER (reachability/patch/glue-logikk) — han fangar IKKJE feil
instruksjons-enkoding i den ~40-instruksjons handkoda rutina. Denne rutina er
difor svakare verifisert enn dei tidlegare omgangane (som spegla prova mønster).
EKTE verifikasjon = Docker/CI ARM64-Linux-loopen (`test_unntakstypar` AOT == tolk).
Fixtur: `tools/fixtures/diff_unntakstype.no`. Enkodingane er kryss-sjekka mot
eksisterande prova instruksjonar, men ikkje køyrt.

## Port

M2/M4/M6/M9-testane AOT == tolk (Docker/CI). `test_unntakstypar` er hovudporten
for M6-dispatchen.
