# Norscode VS Code

This folder contains the bundled VS Code support for Norscode.

It provides:

- syntax highlighting for `.no` files
- language configuration for comments, brackets, and auto-closing pairs
- snippets for common Norscode constructs
- a file icon theme for Norscode source and project assets

## Local Use

The extension expects the Norscode CLI to be available so editor tooling can use:

```text
./bin/nc lsp
```

The current bundle in this repository is file/language support only. LSP is provided by the Norscode runtime and CLI, not by JavaScript extension code in this folder.

## Files

- `package.json` - extension manifest
- `language-configuration.json` - editor language rules
- `syntaxes/norscode.tmLanguage.json` - TextMate grammar
- `snippets/norscode.json` - editor snippets
- `file-icons/norscode-file-icons.json` - icon theme mapping

## Verification

- `./bin/nc lsp` should exit cleanly
- open a `.no` file in VS Code and confirm language detection as `norscode`
- enable the `Norscode File Icons` theme to preview custom icons
