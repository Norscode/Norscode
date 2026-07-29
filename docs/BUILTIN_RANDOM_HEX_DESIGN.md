# Builtin random_hex-design (v821)

## 1) Formål

`builtin.random_hex(n)` skal gi ein trygg, uniform og forutsigbar testbar hex-streng som kan brukast til:
- session-IDar
- tokenar
- cache-nøklar
- unike id-ar i intern diagnostikk

Kravet er at den ikkje må gi forutseielege verdiar, måttele med liten kollisjonsfare, og må vere tydeleg dokumentert i runtime.

## 2) Signatur

- `builtin.random_hex(lengde: heltall)` -> `tekst`
- `lengde` angir talet på hex-teikn i teksten.
- `lengde <= 0` skal returnere tom streng eller kasta kontrollert feil (implementasjonskrav).
- `lengde % 2 != 0` er lov; siste byte blir avrunda ned til nøyaktig tegn-mengde.

## 3) Generasjon i native runtime

1. Hent kryptografisk tilfeldig bytestrøm frå native CSPRNG.
2. Konverter til hex og returner eksakt `lengde` teikn.
3. Aldri bruk pseudo-tilfeldig generator utan kryptografisk styrke.
4. Unngå delte globale RNG-røter på tvers av trådar utan låsing/isolasjon.

## 4) Ytelse og feil

- O( `n` ) i bytes/teikn konvertering.
- Ved CSPRNG-feil skal runtime returnere kontrollert feil, ikkje tom streng.
- Feilmeldingar skal vere generiske (`random source error`) for å unngå lekkasje.

## 5) Sikkerheitsguide (vakt)

- Må aldri eksponere rå intern state.
- Kan berre vere aktivert i DEV dersom stage0 ikkje er modna for produksjon (aktuell no).
- Må ikkje vere deterministic mellom køyringar.
- Må ikkje vere seed-baserte PRNG-ar basert på timestamp/sekund.

## 6) Stage0-policy for no (inntil native klar)

- `v731`-arbeid nyttar fast session-id i DEV og **påstår ikkje** produksjonsklar random.
- Når `random_hex` kjem tilbake i stage0:
  - Erstatt dev-fixtoken med kall til `builtin.random_hex` der mogleg.
  - Oppretthald `production_ready: false` til full sikkerheitsvurdering er ferdig.

## 7) Minimum testtilfelle i testpakke

- `n = 16` returnerer 16 hex-teikn.
- To kall etter kvarandre er ulike.
- Forvent inga panikk ved normal belastning.
- Valider format: berre [0-9a-f].

## 8) Referert API

Designet må gi ein trygg grunnmur for dei vidare oppgåvene:
- `builtin.random_bytes(n)` (rå bytes) som intern støttar og vidarebygging.
- `builtin.uuid()` (formatert UUID/ULID) som kan byggast på `random_bytes`-rot.

## 9) Status

- Dokumentert, ikkje implementert i stage0-kjernen enno.
- Erklåra som «ikke produktionsklar» inntil native støtta er validert.
