# Licensing Norscode

Norscode uses **MPL-2.0 for the compiler and development-tool source identified
below**, with **Apache-2.0 retained for runtime, standard-library, application
template and other explicitly excluded material**. This is a component split,
not an `Apache-2.0 OR MPL-2.0` choice for new MPL-covered changes.

The transition is introduced on 2026-11-13. The last main-branch revision before
this transition is
[`af6cea5e91c2cf235e18025a4bf0d79295320c48`](https://github.com/Norscode/Norscode/tree/af6cea5e91c2cf235e18025a4bf0d79295320c48).
See [Historical Apache rights](#historical-apache-rights) below.

## License texts and precedence

- [LICENSE](LICENSE): the complete Mozilla Public License 2.0.
- [LICENSE-APACHE](LICENSE-APACHE): the previous Apache License 2.0, preserved
  without changing its contents.
- [NOTICE](NOTICE): project provenance and source-availability information.

More specific existing third-party license and attribution notices are retained
and must be followed. The Apache exceptions below take precedence over the MPL
directory list. This document identifies scope; it does not amend either license.

Paths are relative to the Norscode source root. A directory scope includes its
descendants. A `*` in a filename matches within that directory only.

## MPL-2.0 compiler and development-tool scope

The following original Norscode source-code areas, including their build and
development scripts, are governed by MPL-2.0, except for the Apache scopes below:

| Path | Component |
| --- | --- |
| `compiler/` | Compiler frontend, analysis, optimization and toolchain implementation |
| `selfhost/` | Self-hosted compiler, CLI and development tools |
| `cli/` | Command-line tools |
| `tools/` | Build, packaging, validation and other development tools |
| `toolchain/` | Toolchain orchestration and package-manager tools |
| `bin/` | Source launchers and development wrappers |
| `bootstrap/*.no` | Norscode bootstrap source programs |

The following MPL Exhibit A notice applies to the source files in that scope:

> This Source Code Form is subject to the terms of the Mozilla Public License,
> v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain
> one at https://mozilla.org/MPL/2.0/.

Directory-level notices refer back to this scope. Existing separately identified
licenses and the historical Apache rights described below are preserved.

## Apache-2.0 runtime and other retained scopes

The following material remains under Apache-2.0:

| Path or material | Reason |
| --- | --- |
| `runtime/`, `std/`, `stdlib/` | Application runtime and standard libraries |
| `compiler/backend/runtime/`, `compiler/runtime/`, `compiler/vm/` | Runtime support and VM implementations kept separate from the compiler frontend |
| `selfhost/native_execution/` | Native emitters and the runtime implementations embedded in their output |
| `selfhost/std/`, `selfhost/std_native/` | Runtime-facing standard-library support |
| `selfhost/runtime/`, `selfhost/runtime_core/` | Execution runtime |
| `selfhost/network_runtime/`, `selfhost/async_runtime/`, `selfhost/security/` | Network, async and runtime-safety support |
| `selfhost/runtime_*.no`, `selfhost/http_*.no`, `selfhost/vm*.no` | Runtime and VM implementations and support |
| `selfhost/ast.no`, `selfhost/json.no`, `selfhost/semantic_validation_real.no` | Shared support used by the normal VM |
| `selfhost/ncb_bin.no`, `selfhost/ncb_serde.no`, `selfhost/ncb_stream.no`, `selfhost/ncb_runner.no`, `selfhost/l5b_vm_runner.no` | Bytecode serialization, readers and runners |
| `selfhost/std.no`, `selfhost/std_compat.no`, `selfhost/async_io.no`, `selfhost/async_runtime.no`, `selfhost/coroutines.no` | Runtime and standard-library helpers |
| `selfhost/memory.no`, `selfhost/memory_pool.no`, `selfhost/serialization.no`, `selfhost/sandbox.no` | Runtime memory, serialization and sandbox helpers |
| `tools/startproject_templates/` | Application source templates |
| `tools/eksporter_linguist_grammatikk.no` | Grammar exporter and its embedded sample snippets |
| `vscode-norscode/` | Editor grammar, snippets, configuration and extension assets |
| Documentation, examples, tests, images and other non-code assets | Preserve their pre-transition licensing; existing specific notices still apply |
| All other original project material outside the MPL source scopes | Preserve Apache-2.0, including unrelated applications and archived material |

The native emitter files currently mix compiler logic with runtime routines
copied into user executables. Keeping these entire files under Apache-2.0 makes
the output boundary explicit. Moving their compiler-only portions to MPL would
require separating that code in a later change.

An Apache entrypoint can import an MPL compiler module. Each module keeps its
own license: a bundled VM executor or development-tool image can contain both
licenses. This policy does not describe every bundled executable as Apache-only.

## User programs and generated artifacts

Writing or compiling an independent program with Norscode does not assign that
program an MPL license. User-authored code retains the terms chosen by its
rights holders, subject to the licenses of components actually included.

Norscode-supplied application templates and runtime support in the Apache scopes
remain Apache-2.0 when copied into generated projects or executables. This does
not assign Apache-2.0 to the user's own code.

Generated starter projects also bundle Norscode compiler/runtime directories.
The copied license documents govern those bundled Norscode components; they do
not automatically license independently authored application files.

Compiled compiler binaries, bytecode bundles and other generated Norscode
artifacts follow the licenses of their source components. A build-output path
does not grant an additional Apache license to new MPL-covered implementation
code. Pre-transition bootstrap seeds and binaries retain their historical rights.

## Distribution and corresponding source

When distributing a version containing MPL-covered code, follow MPL sections
3.1 and 3.2: make the corresponding covered source available to recipients and
tell them how to obtain it. For an official Norscode release, use the exact
release tag or commit and the source archive for that version at
https://github.com/Norscode/Norscode/releases or the matching commit's archive.
A link to a moving `main` branch alone is not a version-specific source offer.

If you distribute a modified build, make your corresponding MPL-covered source
and changes available as well; an upstream source link alone does not cover
your modifications. The license does not require sending a pull request to
Norscode. Source-sharing duties concern distribution, not merely private use.

Keep `LICENSE`, `LICENSE-APACHE`, `NOTICE` and this scope document with mixed
Norscode distributions. Preserve applicable third-party notices too. An
Apache-only subset, such as the standalone grammar export, carries its Apache
license and relevant provenance rather than being relabeled MPL.

## Historical Apache rights

The transition does not withdraw Apache-2.0 rights already granted in material
previously published under that license. Recipients may continue to use and
fork those earlier versions on their Apache terms.

The existing Apache-origin material retains its applicable Apache license and
notices. Subsequent changes to MPL-scoped files are contributed and distributed
under MPL-2.0 unless an explicit, authorized component-specific license says
otherwise. New MPL changes are not additionally offered under Apache simply
because the file or an earlier version once used Apache.

## Contributions and names

Contributions follow the receiving file or component's declared license.
Contributors must have the rights needed for their contribution and preserve
third-party license and attribution notices. New runtime/template support must
be placed in the appropriate Apache scope if it is to retain that treatment.

Neither license grants a general right to use the project's trademarks. These
license documents do not register a trademark or add new trademark restrictions.
