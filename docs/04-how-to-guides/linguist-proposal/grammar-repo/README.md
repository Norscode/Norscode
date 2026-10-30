# Norscode Grammar — publication staging package

This directory is an **exportable template** for a dedicated public Git repository to hold Norscode's TextMate grammar. It is not an already published grammar repository.

- Official source extension: `.no`
- TextMate scope: `source.norscode`
- License: Apache License 2.0, see `LICENSE`
- Canonical source: [Norscode VS Code grammar](https://github.com/Norscode/Norscode/blob/main/vscode-norscode/syntaxes/norscode.tmLanguage.json)
- Main development PR: [Norscode #274](https://github.com/Norscode/Norscode/pull/274)

## Publication procedure

1. Wait for PR #274 to be reviewed and validated. Do not merge to main automatically.
2. Publish **the contents of this directory** at the root of a new public Git repository (not this monorepo). Confirm proper ownership and licensing before publication.
3. Verify `package.json` refers to the grammar path and that the grammar `scopeName` is `source.norscode`.
4. In a separate checkout of `github-linguist/linguist`, run `script/add-grammar https://github.com/<owner>/<grammar-repo>` **only when GitHub Linguist's independent-usage admission criteria are met**.
5. Run `script/update-ids`, `script/check-regex-compatibility lib/linguist/heuristics.yml`, sample and `test/test_blob.rb` checks, and cross-validation as appropriate in the Linguist repository.
6. Do not claim GitHub supports Norscode until an upstream PR has been accepted and GitHub.com has rolled out the change.

**Sync invariant:** `tests/test_github_linguist_metadata.no` checks that the staged grammar and `LICENSE` are exact copies of their canonical counterparts, so future grammar changes cannot silently diverge within this PR.

**Important:** A new Git repository has **not** been created by this package, and no upstream Linguist files have been modified.
