# Norscode IDE Support — Fase 5

Status: **Under development (June 2026)**

## Language Server Protocol (LSP)

Norscode implements LSP 3.17 for universal editor integration.

### Server Architecture

```
Editor (VS Code / Neovim / etc)
    ↓ JSON-RPC
LSP Server (selfhost/lsp_server.no)
    ↓
Compiler infrastructure (parser, semantic, vm)
    ↓
Information (completions, definitions, diagnostics)
```

### LSP Endpoints

#### initialize
Initialize server connection. Return capabilities.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "initialize",
  "params": {
    "rootPath": "/path/to/project"
  }
}
```

**Response:**
```json
{
  "capabilities": {
    "textDocumentSync": 2,
    "hoverProvider": true,
    "completionProvider": {
      "resolveProvider": true,
      "triggerCharacters": ["."]
    },
    "definitionProvider": true,
    "referencesProvider": true,
    "diagnosticProvider": {}
  }
}
```

#### textDocument/didOpen
File opened in editor.

#### textDocument/didChange
File modified. Incremental or full text sync.

#### textDocument/hover
Show type information and documentation.

**Request:**
```json
{
  "method": "textDocument/hover",
  "params": {
    "textDocument": {"uri": "file:///path/to/file.no"},
    "position": {"line": 10, "character": 5}
  }
}
```

**Response:**
```json
{
  "contents": "funksjon sum(tall: liste[heltall]) -> heltall"
}
```

#### textDocument/completion
Autocomplete suggestions.

**Triggers:**
- `.` (dot) — member access
- Letters — keyword/name completion

#### textDocument/definition
Jump to definition.

**Response:**
```json
{
  "uri": "file:///path/to/file.no",
  "range": {
    "start": {"line": 5, "character": 0},
    "end": {"line": 5, "character": 8}
  }
}
```

#### textDocument/references
Find all references to symbol.

#### textDocument/publishDiagnostics
Push error/warning/info messages to editor.

### Implementation Files

- `selfhost/lsp_server.no` — Main server loop
- `selfhost/lsp_completion.no` — Autocomplete logic
- `selfhost/lsp_navigation.no` — Definition/references
- `selfhost/lsp_hover.no` — Type information
- `selfhost/lsp_diagnostics.no` — Error reporting

## Syntax Highlighting

TextMate grammar (`.tmLanguage.json`) for VS Code, Sublime, Atom.

**Keywords:**
- `funksjon`, `la`, `returner`, `hvis`, `ellers`, `for`, `mens`
- `prøv`, `fang`, `kast`
- `struktur`, `ende`
- `bruk`, `som`

**Types:**
- `heltall`, `desimal`, `tekst`, `bool`
- `liste`, `ordbok`

**Literals:**
- Numbers: `123`, `3.14`, `0x1F`
- Strings: `"hello"`, `'single'`
- Comments: `# single line`, `#{ multi line }#`

## Editor Integration

### VS Code Extension

- Install from Marketplace: `norscode-language`
- Auto-activate on `.no` files
- Provides: syntax highlighting, autocomplete, go-to-definition, hover
- Repository bundle: `vscode-norscode/`

**package.json snippet:**
```json
{
  "activationEvents": ["onLanguage:norscode"],
  "contributes": {
    "languages": [
      {
        "id": "norscode",
        "aliases": ["Norscode"],
        "extensions": [".no"],
        "configuration": "./language-configuration.json"
      }
    ],
    "grammars": [
      {
        "language": "norscode",
        "scopeName": "source.norscode",
        "path": "./syntaxes/norscode.tmLanguage.json"
      }
    ]
  }
}
```

### Neovim

Setup in `init.lua`:

```lua
require'lspconfig'.norscode.setup{
  cmd = { 'nc', 'lsp' },
  filetypes = { 'norscode' },
  root_dir = require'lspconfig'.util.root_pattern('norcode.toml', '.git'),
}
```

### Helix

Setup in `languages.toml`:

```toml
[[language]]
name = "norscode"
scope = "source.norscode"
file-types = ["no"]
language-server = "norscode"

[language-server.norscode]
command = "nc"
args = ["lsp"]
```

### Emacs

LSP mode setup:

```elisp
(add-to-list 'lsp-language-id-configuration '(norscode-mode . "norscode"))
(lsp-register-client
 (make-lsp-client
  :new-connection (lsp-stdio-connection '("nc" "lsp"))
  :major-modes '(norscode-mode)
  :server-id 'norscode))
```

## Performance Considerations

- Debounce textDocument/didChange (100ms)
- Cache symbol table between requests
- Incremental parsing for large files
- Background diagnostics (non-blocking)

## LSP Compliance

- Supports LSP 3.17
- Compatible with Language Server Index Repository (LSR)
- Passes LSP test suite (planned)
