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
