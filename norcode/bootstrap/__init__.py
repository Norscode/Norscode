"""Bootstrap helpers for the legacy Python implementation.

The long-term self-hosting goal is that normal Norscode CLI execution does not
enter the Python compiler path.  Modules in this package are explicit bootstrap
adapters used while the compiler/runtime is migrated to Norscode.
"""
