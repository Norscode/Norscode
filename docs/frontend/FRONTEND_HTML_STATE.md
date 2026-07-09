# Frontend HTML State

`std.html_state` gir små server-renderte markører for UI-state.
Pakken passer når en side er vanlig HTML, men trenger tydelige `data-*` kontrakter for status, bindinger og enkle visningstilstander.

## Markører

- `state(...)`
- `store(...)`
- `bind(...)`
- `computed(...)`
- `app_state(...)`
- `state_attr(...)`
- `bind_attr(...)`

## UI-Hjelpere

- `state_panel(...)`
- `statusfelt(...)`
- `tom_tilstand(...)`
- `laster_tilstand(...)`
- `feil_tilstand(...)`

Eksempel:

```norscode
state.state_panel("Saker", "status", "open", "<p>3 åpne saker</p>")
state.statusfelt("ticket-status", "status", "Oppdatert")
```

Bruk `std.reactive` når state også skal kobles til hendelser, actions eller live bindinger.
