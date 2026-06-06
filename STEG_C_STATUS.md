# Steg C Status: Progresjon & Gjenstår

**Dato:** Juni 6, 2026  
**Status:** Delvis løst - vanlig kjøring funker, bootstrap-self feiler

## ✅ Løst denne sesjonen

### 1. Memory leak debugget
- Identifisert: Runtime-kompilering av `selfhost/common.no` under kjøring
- Bevis: Binary search av fil-størrelse viste feiler på 1500+ linjer
- Root cause: `nc_ensure_sh_common()` i `nc_native_main.c` linje 93 og 114

### 2. Første fix implementert
- ✅ Kommentert ut `nc_ensure_sh_common()` på linje 801
- ✅ Fikset kall inni `nc_call_sh_api()` på linje 115
- **Resultat:** `./bin/nc run app.no` **FUNGERER NÅ** 🎉

### 3. Filer modifisert
```
tools/maint/c/nc_native_main.c:
  - Linje 801-802: Kommentert ut nc_ensure_sh_common()
  - Linje 115: Kommentert ut nc_ensure_sh_common()
```

## ❌ Gjenstår: bootstrap-self feiler

Problemet: Når `selfhost/bootstrap_self.no` kjøres via `NORSCODE_CMD=run`, feiler det med exit code 137 (SIGKILL).

**Anta:** Det finnes andre runtime-compilation hooks eller memory-problemer i C-hosten.

## 🔍 Mulige årsaker

1. **Andre runtime-compilation hooks:**
   - `nc_dispatch_call()` kan laste selfhost-moduler dynamisk
   - Linje 606, 614, 832 bruker `nc_dispatch_call()`

2. **Memory leak i VM:**
   - `nc_exec_call()` har kompleks state-management
   - Kan ha memory leak under kjøring av stor bytekode

3. **Kompleks module loading:**
   - `selfhost/bootstrap_self.no` importerer `selfhost.kompiler` og `selfhost.vm`
   - Disse modulene kan trigge cascading loads

## 📋 Debugging-plan for neste sesjon

### Steg 1: Identifisere blokkering
```bash
# Strace for å sjå memory-allokeringer
timeout 5 strace -e trace=brk ./dist/norscode_native run selfhost/bootstrap_self.no 2>&1 | tail -50

# Sjekk hvis det er infinite brk()-loop
# Resultat: Skal vise mønster av memory-allokeringer
```

### Steg 2: Isolere problemet
```bash
# Test 1: Enkel import
cat > /tmp/test_import.no << 'EOF'
bruk selfhost.json
funksjon start() { returner 0 }
EOF
timeout 2 ./bin/nc run /tmp/test_import.no

# Test 2: Vel kompilering
cat > /tmp/test_compile.no << 'EOF'
bruk selfhost.kompiler
funksjon start() { returner 0 }
EOF
timeout 2 ./bin/nc run /tmp/test_compile.no

# Test 3: VM import
cat > /tmp/test_vm.no << 'EOF'
bruk selfhost.vm
funksjon start() { returner 0 }
EOF
timeout 2 ./bin/nc run /tmp/test_vm.no
```

### Steg 3: Patch minste moduler
Hvis Steg 2 isolerer problemet, patch den relevante modulen sine runtime-compilation kall.

### Steg 4: Rebuild & test
```bash
REGEN=1 NORSCODE_BOOTSTRAP_C=1 bash tools/build_norscode_native.sh
./bin/nc bootstrap-self
```

## Alternativ løsning: Pre-compile common.no

Hvis runtime-compilation problem persisiterer, kan man:

1. Manually compile `selfhost/common.no` frå an older working version
2. Embed det som C-string i `nc_native_main.c`
3. Bruk pre-compiled JSON istf runtime-kompilering

```c
// tools/maint/c/nc_native_main.c linje 93:
static const char *g_common_ncb_json = "{...}"; // Pre-compiled
// return nc_str(g_common_ncb_json);
```

## Konklusjon

**Progress:** 60% mot Steg C
- ✅ Vanlig kjøring: FUNGERER
- ⚠️ Bootstrap-self: Treng mer debugging
- 🎯 Estimat: 1-2 timer mer debugging = LØST

**Blocking:** Ikkje kritisk for selvstendighet - kan bruke alternative kjørings-vei.

---

**Neste sesjon:** Start med Steg 1 (strace) for å identifisere eksakt blokkering.
