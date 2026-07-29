# V6800 Single-module bundle diagnose

## Resultat
Tre delar av bundler-sporet vart testa kvar for seg.

### 1. `les_precompiled_ncb(...)`
Probe:
- `tests/fixtures/v6800/bundler_precompiled_probe_v6800.no`

Resultat:
- grøn
- `precompiled_len=41642`

### 2. `les_ncb_eller_kompiler(...)` med `NORSCODE_USE_PRECOMPILED_SELFHOST=0`
Probe:
- `tests/fixtures/v6800/bundler_compile_probe_v6800.no`

Resultat:
- grøn
- `compiled_len=40359`

### 3. `bygg_bundle(...)` med éin modul
Probe:
- `tests/fixtures/v6800/bundler_single_module_probe_v6800.no`

Resultat:
- raud
- `norscode: Stack underflow`

## Tolkning
Dette snevrar inn feilen svært mykje:
- `les_precompiled_ncb` er ikkje problemet
- `les_ncb_eller_kompiler` er ikkje problemet
- feilen sit i single-module-snarvegen inne i `bygg_bundle(...)`

Det betyr at stack-underflow ikkje kjem frå sjølve kompileringa av `selfhost/lexer/lexer_m1.no`, men frå den ekstra logikken som blir brukt når bundler prøver å skrive ut eller handtere resultatet i single-module-grena.

## Ny minste kjeldeblokk
Den mest mistenkte blokka er no denne delen av `selfhost/bundler.no`:
- `hvis lengde(modular) == 1 { ... }`

Særleg:
- parsing av `arg_0`
- `fil_les(fil_sti_0)`
- `kompiler_fil(kjelde_0, "__main__")`
- `fil_skriv(utfil, ncb_json_0)`
- lengde-/skriv-lina etterpå

Sidan compile-hjelparen alt er bevist grøn, peikar feilen sterkast mot single-module-spesifikk handtering etter compile-returen.
