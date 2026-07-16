# SQLite amalgamation

Norscode vendors the official SQLite 3.53.3 amalgamation so native runtime
builds do not depend on an operating-system SQLite development package.

- Source: `https://www.sqlite.org/2026/sqlite-amalgamation-3530300.zip`
- Archive SHA3-256: `d45c688a8cb23f68611a894a756a12d7eb6ab6e9e2468ca70adbeab3808b5ab9`
- Included files: `sqlite3.c`, `sqlite3.h`
- License status: SQLite is in the public domain.

Runtime builds should define `SQLITE_THREADSAFE=1`,
`SQLITE_DQS=0`, `SQLITE_DEFAULT_MEMSTATUS=0`, and
`SQLITE_OMIT_LOAD_EXTENSION` unless a reviewed build profile explicitly needs
different behavior.
