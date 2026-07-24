# exec_prosess policy (v831)

This document defines how `builtin.exec_prosess` shall be used in stage0 and later, with a safe-by-default model.

## 1) Scope and risk

`builtin.exec_prosess` executes shell/process commands. This can:
- run arbitrary code
- read/write local files
- make network calls through shell tools
- leak environment and secrets

Default rule: deny unless explicitly allowed.

## 2) Policy goals

1. Safe by default.
2. DEV clearly separated from production.
3. Deterministic audit trail.
4. Ability to disable/kill from environment policy.

## 3) Default gate model

- Disabled by default in production mode.
- Allowed in DEV mode only with explicit runtime flag.
- Flag example: `ALLOW_EXEC_PROSESS=1` (or equivalent config switch).
- In production, runtime must return a clear controlled error for every call.

## 4) Command model

Only allow:

- Whitelisted command names from `ALLOWED_EXEC_COMMANDS`.
- Optional fixed argument patterns only.
- No raw shell concatenation.

Not allowed by default:

- Shell metacharacters (`;`, `|`, `&&`, `||`, `$(...)`, backticks, redirects from file descriptors).
- `sh`, `bash`, `cmd`, `powershell` wrappers.
- User-provided command strings.
- Commands with unbounded input from request data.

Implementation pattern:

1. Receive command as string.
2. Parse into executable + args with strict parser.
3. Validate executable is in allowlist.
4. Validate args against allowlist/patterns.
5. Execute with a minimal environment.
6. Return output and exit code in a bounded structure.

## 5) Timeout and resource limits

- Command timeout: hard cap (example 2–5 seconds for short operations).
- Output size cap: capped stdout/stderr (example 64KB total).
- No interactive mode.
- No process group escaping or background daemonizing.

## 6) Logging and audit

Every call should be logged with:
- caller context (route/module)
- command signature
- allow/deny decision
- exit code
- duration
- truncated output metadata

No raw secrets from env or output should be logged.

## 7) Kill switch

- Support global kill switch: `DISABLE_EXEC_PROSESS=1`.
- Support runtime reload of policy (where feasible).
- If kill switch active, all calls fail closed.

## 8) Error semantics

- When denied: return controlled error, never execute.
- When enabled but invalid command/args: return controlled validation error.
- Never throw raw shell error output upstream.

## 9) Migration plan

- Phase 1: Keep current high-risk calls behind explicit module-level feature flags.
- Phase 2: Replace unsafe runtime string calls with safe helpers in `std`.
- Phase 3: Remove broad command usage from std modules not required in stage0.

## 10) Stage0 status check

- For now, this repo notes `native` stage0 should treat `exec_prosess` as restricted/disabled.
- If feature is needed, it must run via this policy and explicit allowlist.

