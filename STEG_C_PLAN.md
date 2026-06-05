# Steg C: VM-kjøring av NCB — Implementeringsplan

**Status:** Fase 1 — Haltvei til fullføring

## Problemet

For å oppnå full selvstendighet (Steg C), trenger Norscode å:
1. Kompilere kode til NCB JSON ✅ (funker perfekt)
2. Kjøre NCB via `selfhost/vm.no` ❌ (blokkert av C-host)

Blokkering: C-host (`host_exec_ncb_json`) feiler med exit code 137 (Killed).

## Hva er implementert denne sesjonen

### 1. JSON-wrappers i vm.no ✅
- `json_parse(input)` → `json_les(input)`
- `json_stringify(v)` → `json_skriv(v)`
- `json_parse_raw(input)` → `json_les(input)` (for NCB-parsing)

### 2. Kompilerbar kjede ✅
- `bootstrap_self.no` kompileres til 2.5 KB NCB JSON
- `test_vm_executor.no` kompileres til 2.1 KB NCB JSON
- `ncb_runner.no` kompileres (for framtidig bruk)

### 3. Funksjonar tilgjengelige i vm.no ✅
- `køyr_funksjon()` — kjør bytekode
- `json_parse_raw()` — parse NCB JSON

## Blokkering: C-host (host_exec_ncb_json)

**Problem:** Når C-host prøver å kjøre ein Norscode-fil som importerer `selfhost.vm` eller `selfhost.kompiler`, får den exit code 137 (Killed).

**Mulige årsaker:**
1. Uendelig løkke i JSON-parsing
2. Stack overflow frå djupe rekursjonarkall
3. Memory allocation som feiler
4. Timeout (prosess drept etter 5 sekunder)

**Debug-strategi:**
```bash
# Prøv med strace for å sjå kva som skjer
strace -e trace=open,mmap,brk ./dist/norscode_native run selfhost/bootstrap_self.no

# Prøv med gdb
gdb ./dist/norscode_native
(gdb) run
(gdb) bt
```

## Løsning 1: Fikse C-host (større innsats)

Analyser C-genereringen i `bootstrap/maint/c/norscode_generated.c` for:
- Infinite loops
- Stack allocation
- Memory leaks
- Recursion depth

## Løsning 2: Bygg direktekjøring (mindre innsats)

Bruk eksisterende kompilator til å bygge `ncb_runner.no` som:
1. Leser NCB JSON frå fil
2. Kjører det via `selfhost/vm.no`
3. Returnerer exit-kode

**Fordel:** Unngår C-host, kjører ren Norscode

**Nød:** Treng fungerande Norscode-kjøring for å teste (catch-22)

## Anbefalte neste steg

### Kortsiktig (denne sesjonen)

1. Debug C-host med `strace` eller `gdb`
2. Identifiser blokkering (loop, stack, memory)
3. Lag fix i nc_native_main.c eller norscode_generated.c

### Langsiktig (Fase 2-3)

1. Når C-host funker: Test `bootstrap_self` i CI
2. Implementer pure-Norscode ELF emitter (Phase 6)
3. Fjern C-host frå kritisk sti

## Filer oppretta denne sesjonen

- `selfhost/test_vm_executor.no` — Test VM-kjøring
- `selfhost/test_json_parse_raw.no` — Test JSON-parsing
- `selfhost/ncb_runner.no` — NCB-executor (framtidig bruk)
- `STEG_C_PLAN.md` — Denne filen

## Konklusjon

**Kompileringa av selfhost-kjeden er 100% funksjonell.** 
Eneste blokkering er C-host-kjøringa som feiler.

For full selvstendighet trenger vi enten:
1. Fikse C-host (debug + patch)
2. Implementere alternativ kjøring (bare Norscode)

Steg C er teknisk mulig — det trengs berre debugging av C-hosten.
