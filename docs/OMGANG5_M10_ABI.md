# Omgang 5 · M10 i AOT — varargs, standardarg, spread, destrukturering

**Mål:** køyre M10-argumentfunksjonar AOT-native, byte-likt med tolk-orakelet.

## Den store innsikta

Det MESTE av M10 er allereie kompilert til **vanleg bytekode** av kjelde-
kompilatoren (`ir_to_bytecode.no`) — native treng ingen nye opcodar for det:

- **Standardargument** (`:2152`) — kompilatoren emitterer ein *prolog i sjølve
  funksjonskroppen*: `LOAD_NAME p; PUSH_CONST "__norscode_unset__"; COMPARE_EQ;
  JUMP_IF_FALSE skip; <default>; STORE_NAME p; LABEL skip`. Reine opcodar native
  alt støttar. Fungerer så snart manglande argument kjem inn som sentinelen.
- **Destrukturering** (`:1796`) — `la [a,b] = e` → `tmp=e; a=tmp[0]; b=tmp[1]`
  (INDEX_GET). Vanleg bytekode.
- **Spread** (`:1299`) — kallstaden byggjer argumentlista dynamisk (BUILD_LIST +
  konkat). Vanleg bytekode.

Så den einaste NATIVE-spesifikke jobben er dei to tinga `bl` ikkje ber
argumenttal for: **rest-pakking** og **sentinel for manglande argument**.

## Oppgåve 1 — rest-param + standardarg-tilpassing *(GJORT)*

`bl` ber ikkje argumenttal, så tilpassinga skjer på **KALLSTADEN** (som kjenner
argumenttalet + målsignaturen via `funksjonar[mål]`), ikkje i callee-prologen.
Callee får alltid nøyaktig `nparams` register-argument. I CALL-emittaren:

- **rest_param** (metadata `"rest_param":"sann"`): params[0..nfixed-1] faste,
  params[nfixed] = rest-liste. Pakk operandane `[djupn-nargs+nfixed .. djupn-1]`
  i ei NcVal-liste (`emit_build_list`/`emit_build_list_mem` → x15), flytt faste
  argument til x0..x[nfixed-1], `mov x[nfixed], x15`.
- **nargs < nparams** (manglande argument): flytt dei `nargs` reelle argumenta,
  pad `x[nargs..nparams-1]` med `emit_ncval_str_const("__norscode_unset__")`.
  Standardarg-prologen i callee-kroppen byter sentinelen mot defaulten. (Samsvarar
  med tolken, som òg set manglande argument til `"__norscode_unset__"`, vm.no:4491.)
- **normal** (nargs == nparams, ingen rest): **byte-identisk** med før → fixpunktet
  + alle eksisterande kall er urørte.

Alle tre greinene deler same `bl` + resultat + `djupn = djupn - nargs + 1`.

**Strukturelt verifisert:** kjelde-codegen kompilerer eit NCB med både ein
default-funksjon (kalla med for få argument) og ein rest-funksjon → image utan
feil. Maskinkode-enkoding = Docker/CI. MERK: seed-frontenden parsar ikkje
M10-syntaks (`b = 10`, `...resten`) enno, så differensial via `bygg-native`
ventar på seed-rebuild; tolk-sida (kjelde-kompilator) + strukturell codegen dekkjer
logikken. Fixturar: `tools/fixtures/diff_default.no`, `diff_varargs.no`.

## Oppgåve 2 — spread/destrukturering/optional/template *(neste)*

Sidan desse alt er vanleg bytekode (over), er forventinga at native ALT køyrer
dei — oppgåve 2 = verifisere med fixturar (differensial grøn) og tette
eventuelle attståande opcode-hol (t.d. optional-chaining/template-spesifikke
opcodar om nokon finst). `test_spread/destrukturering/optional/template` AOT == tolk.
