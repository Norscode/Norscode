# Audit- og security-mønster

Bruk `std.audit` for hendelser som bør være tydelige, sporbare og egnet for sikkerhetslogg.

## Typiske hendelser

- `auth_fail(...)`
- `access_denied(...)`
- `sensitive_read(...)`
- `sensitive_write(...)`

Eksempel:

```norscode
bruk std.audit som audit

funksjon start() -> heltall {
    la e = audit.access_denied("ada", "admin", "/admin")
    audit.felt_bool(e, "blocked", sann)
    audit.emit(e)
    returner 0
}
```

Audit-hendelser er bevisst tekstbaserte og egner seg godt som JSON-linjer i sikkerhetslogger.
