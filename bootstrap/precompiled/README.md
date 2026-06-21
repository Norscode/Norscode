# Pre-compiled Bootstrap Modules

Pre-compiled bytecode (NCB JSON) for large selfhost modules to avoid runtime compilation overhead.

**Files:**
- `lexer_m1.ncb.json` — lexer tokeniser for bootstrap path (41 KB)
- `json.ncb.json` — JSON parsing/stringifying (15 KB)
- `parser.ncb.json` — Norscode parser (91 KB)
- `semantic.ncb.json` — Semantic analysis (11 KB)
- `ir_to_bytecode.ncb.json` — IR to bytecode compilation (136 KB)
- `stil.ncb.json` — norsk stilarkmodul for `std.stil`

**Usage:**
These files are loaded by the bootstrap system to avoid runtime compilation of large modules. The bootstrap gates and bundler can reference these pre-compiled bytecodes instead of compiling from source.

**Building:**
```bash
./bin/nc compile selfhost/lexer/lexer_m1.no bootstrap/precompiled/lexer_m1.ncb.json
./bin/nc compile selfhost/json.no bootstrap/precompiled/json.ncb.json
./bin/nc compile selfhost/parser.no bootstrap/precompiled/parser.ncb.json
./bin/nc compile selfhost/compiler/semantic.no bootstrap/precompiled/semantic.ncb.json
./bin/nc compile selfhost/compiler/ir_to_bytecode.no bootstrap/precompiled/ir_to_bytecode.ncb.json
./bin/nc compile std/stil.no bootstrap/precompiled/stil.ncb.json
```

**Note:** common.no (2404 lines) is too large to pre-compile in current C-host (OOM). It requires C-host memory optimization (Task #15).

---
Generated: 2026-06-06
