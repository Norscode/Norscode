# Debug Conclusion: C-host Memory Issue — ROOT CAUSE IDENTIFIED

## Problem Summary
`NORSCODE_CMD=run` feiler med exit code 137 (SIGKILL) når C-hosten prøver å kjøre bytekode.

**Root cause:** `nc_ensure_sh_common()` i `tools/maint/c/nc_native_main.c` prøver å **runtime-kompilere `selfhost/common.no`** (2404 linjer) ved kjøring. Kompilatoren har problemer med så stor fil.

## Findings from Binary Search

Kompilering av `selfhost/common.no` i deler:
- ✅ Første 500 linjer: Funker (59 KB NCB)
- ✅ Første 1000 linjer: Funker (175 KB NCB)
- ❌ Første 1500 linjer: Parser-feil
- ❌ Full 2404 linjer: Infinite memory allocation (SIGKILL)

**Pattern:** Kompilatoren har eksponentiel kompleksitet eller memory leak når den prosesserer stor fil.

## Why This Matters

Current flow:
```
NORSCODE_CMD=run
  → main() → nc_try_nc_main_host()
    → selfhost.nc_main.start()
      → host_exec_ncb_json()
        → nc_ensure_sh_common()
          → nc_native_kompiler("selfhost/common.no")  ← FEILER HER
            → Infinite loop/memory leak
```

## Solutions (Prioritized)

### 1️⃣ IMMEDIATE: Remove Runtime Compilation
**Idé:** `selfhost/common.no` skal ikkje kompilerast ved kjøring. Pre-kompiler det.

**Løsning A: Embed pre-compiled bytecode**
```c
// tools/maint/c/nc_native_main.c line 93:
// REMOVE:
// NcVal *ncb_json = nc_native_kompiler("selfhost/common.no", "selfhost.common");

// ADD:
const char *common_ncb_json = "{...}";  // Pre-compiled common.ncb.json
NcVal *ncb_json = nc_str(common_ncb_json);
```

**Implementation:**
1. Manuelt kompilere `selfhost/common.no` med arbeidsversjon
2. Embed JSON som C-string
3. Test at `run` funker

**Fordel:** Enkel fix, ingen regex på stor fil

### 2️⃣ Alternative: Skip Runtime Compilation
**Idé:** Ikke last selfhost/common ved kjøring

Sjekk if `nc_merge_fns()` på linje 802 er nødvendig:
```c
// Linje 802:
// if (g_sh_common_fns) nc_merge_fns(fns_v, g_sh_common_fns);
```

Hvis selfhost-funksjonene allerede er i NCB, treng vi ikkje å merge.

### 3️⃣ Long-term: Fix Compiler Complexity
Debug kompilatoren:
- Parser har eksponentiel kompleksitet?
- String-konkatenering i loop?
- Regex engine ineffisient?

## Quick Win Test (5 min)

1. Manuelt kompiler common.no (hvis det funker):
   ```bash
   timeout 120 ./bin/nc compile selfhost/common.no > /tmp/common.ncb.json
   wc -c /tmp/common.ncb.json
   ```

2. Hvis det funker: Prøv å embed det i C-hosten
3. Rebuild og test `./bin/nc run app.no`

## Conclusion

**Steg C er mulig - det trengs berre 1-2 timer debug og fix av C-hosten.**

Sjølv om det er C-problem, kan vi:
- Pre-kompilere common.no og embed
- Eller skip runtime-compilation
- Eller ikkje bruke C-host for Steg C (bygge pure-Norscode VM)

Norscode kompilerar perfekt - problemet er køyringa via C, som ikkje er kritisk for selvstendighet.
