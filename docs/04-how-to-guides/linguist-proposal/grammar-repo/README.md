# Norscode TextMate Grammar — export instructions

This is an **export plan**, not a published repository. No generated JSON or duplicate
license is stored in this tracked documentation directory.

## Generate the standalone grammar package

From the root of a checkout of `Norscode/Norscode`:

```sh
./bin/nc run tools/eksporter_linguist_grammatikk.no
```

The command is written in Norscode. It creates these **generated** files under
`build/norscode-linguist-grammar/`:

- `package.json`: small VS Code/TextMate grammar manifest
- `syntaxes/norscode.tmLanguage.json`: copied from `vscode-norscode/syntaxes/norscode.tmLanguage.json`
- `LICENSE`: copied from the repository's Apache-2.0 root license
- `README.md`: copied from this document

The exporter performs read-back comparisons and returns a non-zero exit code
if the generated grammar or license differs from the canonical originals.
GitHub CI runs this Norscode exporter on pull requests.

## Publishing the resulting folder

1. Inspect the generated package and its license.
2. When Norscode meets GitHub Linguist's independent usage/admission criteria,
   publish **the contents of** `build/norscode-linguist-grammar/` as the root of
   a *separate* public repository. This step has **not** been done.
3. From a separate checkout of `github-linguist/linguist` run
   `script/add-grammar https://github.com/<owner>/<grammar-repo>`.
   This imports a Git submodule, so passing the entire Norscode repository is incorrect.
4. Add `languages.yml` and `heuristics.yml` definitions, real samples and
   attribution; execute `script/update-ids`,
   `script/check-regex-compatibility lib/linguist/heuristics.yml`,
   `bundle exec rake test` and `bundle exec script/cross-validation --test`.
5. Submit an upstream PR only with independently verified Norscode adoption
   and required sample/grammar evidence.

**Scope:** `source.norscode`, **extension:** `.no`,
**license:** Apache-2.0.

Official recognition in GitHub does not occur until the Linguist maintainers
accept a change and GitHub.com deploys the relevant Linguist release.

## Real Norscode samples for upstream

The exporter also creates `build/norscode-linguist-upstream/` as a **separate**
staging area for the future GitHub Linguist PR. Its
`samples/Norscode/` subdirectory contains byte-identical copies of four real
source files from this repository, renamed for clarity:

- `compiler_runtime.no` from `compiler/compiler.no`
- `toml_parser.no` from `std/tomllib.no`
- `ghash.no` from `std/ghash.no`
- `package_cache.no` from `toolchain/norspkg/cache.no`

`SAMPLE_ORIGINS.md` records original source paths, repository links, and the
Apache-2.0 repository license. Verify every file's own headers before
submitting upstream, and attach an exact source commit and original links.

Copy these files to `samples/Norscode/` in the separate **upstream Linguist
checkout**, not into the standalone grammar Git repository. Add genuine
`Text` examples for the shared `.no` extension when needed by Linguist's
sample/heuristics tests.

**These four files are all controlled by the same Norscode project and do
not provide evidence for independent public adoption.** They are regression
and classification samples only.

## Downloading the CI-generated package

The `Format og Lint (Python-fri)` job uploads the two exported folders as
one GitHub Actions artifact named **`norscode-linguist-proposal`**.
Open the successful CI run for PR #274, select that job's workflow run,
and download the artifact from its **Artifacts** section.

The artifact contains both `norscode-linguist-grammar/` and
`norscode-linguist-upstream/` (including `SAMPLE_ORIGINS.md`).
Artifacts are retained for **14 days**, not committed to the source tree.

For pull requests, GitHub Actions sets `NORSCODE_LINGUIST_SOURCE_SHA` to
`github.event.pull_request.head.sha` so the generated sample source URLs
point to the actual **PR head commit**, not GitHub's temporary merge commit.
The exporter refuses a CI run without a 40-character source SHA. For
local export without that variable, `SAMPLE_ORIGINS.md` explicitly marks
links as unpinned; supply a real source commit before publishing samples.

A successful Norscode export and artifact upload do **not** mean upstream
GitHub Linguist has accepted this language, nor that the independent-use
threshold has been met.
