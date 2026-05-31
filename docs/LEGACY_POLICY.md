# Legacy Policy

This repository uses `Norscode` as the primary product name.

See also [`docs/SELFHOST_MIGRATION_AND_DEPRECATIONS.md`](./SELFHOST_MIGRATION_AND_DEPRECATIONS.md) for the current migration path and reading order.

## Canonical names

- User-facing product name: `Norscode`
- historisk vei package namespace: `norcode`
- Primary CLI command: `norcode`

## Allowed legacy names

`norsklang` may remain only where it preserves compatibility with older projects or existing users.

Allowed legacy uses:

- CLI wrappers that forward to `norcode`
- historisk kompatibilitetsentrypoint for eldre installasjoner
- old config and cache file names when reading or migrating existing projects
- import/package compatibility shims that keep older installs working

## Names that should not use `norsklang`

Use `Norscode` instead of `norsklang` in:

- README and docs
- release notes
- help text and user-facing errors
- examples and templates
- new code comments unless the comment is explicitly about legacy compatibility

## File and config guidance

- New project files should use `norcode.toml`
- Legacy migration code may still read `norsklang.toml`
- New caches and generated files should use `.norcode/`
- Legacy migration code may still read `.norsklang/`

## Practical rule

If a name is visible to a new user, it should be `Norscode`.
If a name exists only to keep older projects working, `norsklang` is acceptable.
