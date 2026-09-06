# Plan: frå raud CI til 100 % Norscode (seed-promotering)

Levande statusplan. Oppdatert av kvar commit som endrar status. Sist oppdatert: **2026-09-06 (kveld, 19:15)**.

Mål (brukaren): *«når alt er ferdig skal det bare være norscode igjen. ingen c, python eller json»*.
Vegen dit går gjennom **fire fasar** som må takast i rekkjefølgje. Kvar fase har ein målbar
«ferdig-når»-test.

Teiknforklaring: `[x]` ferdig · `[~]` pågår · `[ ]` ikkje starta · `[!]` blokkert

---

## Fase A — CI grønn på PR #186 (`selvstendig-fra-179` → `main`)

Ferdig når: alle jobbar i `ci.yml`, `gc-litmus.yml` og `b2-seed-direct.yml` er grøne på same commit.

| Jobb | Status | Årsak / tiltak |
|---|---|---|
| Fast lanes (Linux/macOS) | `[x]` grøn | — |
| GC-litmus (10k, sys6, nulltype, eqnull, strnull) | `[x]` grøn | — |
| ELF stage-0 fixpunkt (Gen1 == Gen2) | `[x]` lokalt | Fragment regenerert i Docker; ny køyring: NCB 704393 == 704393, ELF 1097696 identiske (BESTÅTT). Verifiserast i CI |
| Slow tests Linux | `[~]` | var exit 143 (OOM) etter 96 min utan logg. No: 2 runnarar (matrix) + strøymd logg (A1). Verifiserast i neste CI-runde |
| Slow tests macOS | `[~]` | var 300 min timeout + 22 forlatne prosessar. No: 3 runnarar (matrix), SIGKILL ved test-timeout, strøymd logg (A2). Verifiserast i neste CI-runde |
| B2 «Fullhost nc_main native seed» | `[~]` | `Ukjend NORSCODE_CMD: selfcompile-l5` — fiks (`run tools/selfcompile_l5.no`) ligg lokalt, går med neste push |

### Tiltak
- **A1 Linux-OOM diagnose** `[x]` kode / `[ ]` verifisert — `ci_shell_runner.no` strøymer barnet sitt stdout (async spawn + wait/read) når `NORSCODE_VM_CI_STREAM=1`; `nc_test_parallel.no` drenerer shard-output kvar 5. sekund. Neste runner-død viser kva test som køyrde.
- **A2 macOS forlatne prosessar** `[x]` kode / `[ ]` verifisert — Lokalt lek ingen av async-/daemon-testane; kjelda er compile-steg som gjekk ut på tid utan å bli drepne. `nc_test.no` køyrer no testbarnet via async-ABI-en og sender SIGKILL ved timeout. I tillegg er lanen delt på 3 runnarar (`NC_PARALLEL_SHARD_ONLY`).
- **A3 Kompilator: builtin skal vinne over ukvalifisert import** `[x]` — `legg_til(l, x)` i `__main__` vart `CALL std.dns.legg_til` når `std.dns` var importert (`imported_funk_kart`). Fiks i `ir_to_bytecode.registrer_importerte_funksjonar` + utvida `semantic.er_builtin`. Verifisert på seed AE: `test_dns_ds_record` OK. Fragment regenerert og fixpunkt BESTÅTT lokalt (commit 39aaef1).
- **A4 Push + ny CI-runde** `[~]` — 18 commits pusha 2026-09-06 kveld; B2 fullhost dispatcha. Ventar på: CI (slow-lanes matrix, ELF-fixpunkt), GC-litmus, B2 seed-port-tabell.
- **A5 Linux-async-backend** `[ ]` (valfri) — committed Linux-stage0 sin async-spawn ignorerer environment-kartet. Ikkje blokkerande (adapteren bind miljøet sjølv), men bør fiksast i native_gap/process når seeden blir promotert.

---

## Fase B — fersk self-hosta x86-64-seed passerer seed-porten

Ferdig når: `tools/seed_gate_tests.txt` (97 testar) køyrer grønt på seed bygd av
`tools/build_x86_seed.no` i CI-jobben «Fullhost nc_main native seed».

| Delmål | Status | Merknad |
|---|---|---|
| Seed byggjer og køyrer tiny42/B2-subset | `[x]` | 6/6 lokalt |
| zip/tar/filops/media/shutil/process/socket/network/DNS/json | `[x]` | passerer på seed AA–AE |
| `test_dns_ds_record` | `[x]` | rot-årsak var A3 (kompilatorfeil); grøn på seed AE |
| `test_template` (verts-VM via host_kall) | `[x]` flytta | «Ukjent variabel: f» — feilar òg på committa VM → språkparitet (f-strengar). Flytta til `language_parity_tests.txt` |
| `test_security` | `[!]` | timeout 400 s på seed — må profilerast (neste) |
| Seed-port-tabell i CI (B2 fullhost) | `[ ]` | ventar på A4 |
| Native `desimaltall` (flyttal) | `[ ]` | 2–3 veker om det skal inn i porten; elles utanfor |
| `db.*` (NorsDB rein Norscode) | `[ ]` | eige spor |
| tls / sandbox-profilar / trådar | `[ ]` | eige spor |

---

## Fase C — promotering og sletting

Ferdig når: `bootstrap/stage0/*` er bygd av Norscode frå kjelde, og `archive/`-C, hex-blobar,
Python-verktøy og JSON-artefaktar er sletta utan at CI blir raud.

1. `[ ]` Promoter fersk seed til `bootstrap/stage0/norscode-linux-x86_64` (etter Fase B).
2. `[ ]` Regenerer `bin/nc`/`dist` frå promotert seed på alle plattformer.
3. `[ ]` Slett C-arkivet, frosne hex-runtime-blobar (RT_*), C-æra-seedar.
4. `[ ]` NCB JSON → binær (`ncb_serde` dual-format er alt på plass; skru om skrivaren).
5. `[ ]` Slett Python-hjelparar og legacy-shell.

---

## Fase D — språkparitet (kan gå parallelt i eigne sesjonar)

- `[ ]` f-strengar (`test_template`), `tools/language_parity_tests.txt` (11 testar, Parserfeil på alle seedar)
- `[ ]` `desimaltall` nativt
- `[ ]` NorsDB → SQLite-kompatibel kjerne

---

## Nå-kø (kva som køyrer akkurat no)

| Jobb | Kvar | Forventa |
|---|---|---|
| CI-runde på pusha HEAD (slow-lanes matrix, ELF-fixpunkt, GC-litmus) | GitHub | ~3–4 t |
| B2 fullhost (L5 + materialize + seed-port 97 testar) | GitHub | ~3 t |
| `test_security` på seed AE: html-delen OK på 0 s; PBKDF2- og rekursjonsdelen under måling | Docker | — |

## Logg

- 2026-09-06 19:40: Fixpunkt BESTÅTT lokalt med regenererte fragment (Gen1 == Gen2). Alt pusha (18 commits), B2 fullhost dispatcha. Linux-stage0 sin async-backend ignorerer environment-kartet → adapteren bind miljøet sjølv (env.write), harness-async berre på macOS.

- 2026-09-06 19:15: A1/A2-kode ferdig og verifisert lokalt (runner-strøyming, shard-only, SIGKILL ved timeout). Slow-lanes delt i matrix (macOS 3, Linux 2). `test_template` ut av seed-porten. Fragment regenerert (ir_to_bytecode + semantic endra, resten byte-identisk).
- 2026-09-06 18:50: A3 rot-årsak funne (`imported_funk_kart` skuggar builtin) og verifisert på seed AE. Seed AD bygd med ny nc_main + vm (host_kall-fallback verifisert: `test_template` når no VM-en). CI-runde på ae13f62: begge slow-lanes raude, B2 fullhost raud (gamal kommando).
