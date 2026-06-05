# Norscode Package Registry Protocol

Status: **Specification (Fase 4)**

## Overview

Registry is HTTP-based, compatible with semantic versioning and distributed caching.

Default registry: `https://registry.norscode.dev`

## API Endpoints

### GET /v1/packages/{name}

List all versions of a package.

**Request:**
```
GET /v1/packages/http
Authorization: Bearer {token}  # Optional
```

**Response (200 OK):**
```json
{
  "name": "http",
  "versions": [
    {
      "version": "1.0.0",
      "published": "2026-01-15T10:00:00Z",
      "yanked": false
    },
    {
      "version": "1.2.3",
      "published": "2026-06-01T14:30:00Z",
      "yanked": false
    }
  ]
}
```

### GET /v1/packages/{name}/{version}

Get metadata for specific version.

**Request:**
```
GET /v1/packages/http/1.2.3
```

**Response (200 OK):**
```json
{
  "name": "http",
  "version": "1.2.3",
  "description": "HTTP client library",
  "authors": ["Alice <alice@example.com>"],
  "license": "Apache-2.0",
  "repository": "https://github.com/example/http",
  "dependencies": {
    "json": "^2.0"
  },
  "hash": "sha256:abc123def456...",
  "download_url": "https://registry.norscode.dev/dl/http/1.2.3/http-1.2.3.tar.gz",
  "signature": "sig_base64encoded..."
}
```

### POST /v1/publish

Publish a new package version.

**Request:**
```
POST /v1/publish
Authorization: Bearer {token}
Content-Type: application/octet-stream

[package tarball binary content]
```

**Headers:**
- `X-Package-Name`: name of package
- `X-Package-Version`: semantic version
- `X-Signature`: Ed25519 signature of tarball

**Response (201 Created):**
```json
{
  "message": "Published http 1.3.0",
  "url": "https://registry.norscode.dev/packages/http/1.3.0"
}
```

**Response (409 Conflict):**
```json
{
  "error": "Version already exists"
}
```

### GET /v1/search

Search packages.

**Request:**
```
GET /v1/search?q=http&limit=20
```

**Response (200 OK):**
```json
{
  "results": [
    {
      "name": "http",
      "version": "1.2.3",
      "description": "HTTP client library",
      "downloads_week": 1250
    }
  ]
}
```

## Authentication

### Token-based (for publish)

User obtains token from `registry.norscode.dev/auth`:

```bash
nc auth --registry https://registry.norscode.dev
# Opens browser, returns token
```

Token stored in `~/.norscode/credentials.toml`:

```toml
[registry]
token = "nc_..."
```

### Verification

All packages are signed with Ed25519. Client verifies:

```
signature = Ed25519(tarball, author_public_key)
```

## Package Format

Package tarball structure:

```
http-1.2.3.tar.gz
├── norcode.toml        # Manifest
├── src/
│   ├── main.no
│   └── util.no
├── examples/
│   └── client.no
├── LICENSE
└── README.md
```

## Semver Resolution

Dependency constraints:

| Constraint | Meaning |
|-----------|---------|
| `1.2.3` | Exact version |
| `^1.2.3` | >= 1.2.3, < 2.0.0 (caret) |
| `~1.2.3` | >= 1.2.3, < 1.3.0 (tilde) |
| `1.2.*` | >= 1.2.0, < 1.3.0 (wildcard) |
| `>=1.2.0, <2.0.0` | Range |

### Transitive dependency resolution

Algorithm (deterministic):
1. Collect all direct dependencies
2. For each dependency, fetch its manifest
3. Recursively resolve transitive deps
4. Detect conflicts (same package, incompatible versions)
5. Choose highest compatible version for each package
6. Pin in `norcode.lock`

## Security Considerations

1. **Signature verification** — All packages must be signed
2. **HTTPS only** — Registry uses TLS
3. **Token scope** — Tokens can be limited to specific packages
4. **Yanking** — Owners can yank versions (marked in registry, not deleted)
5. **Audit log** — All publishes logged per package

## Implementation Checklist

- [ ] HTTP client in `selfhost/registry_client.no`
- [ ] Semver resolver in `selfhost/resolver.no`
- [ ] Package publisher in `selfhost/publisher.no`
- [ ] Signature verification in `selfhost/crypto.no`
- [ ] Default registry at `registry.norscode.dev`
