# Mac Native Window Omganger

Dette dokumentet dekkjer det som framleis manglar for at Norscode-appen på macOS skal opne eit **ekte app-vindauge**, ikkje berre Terminal rundt den bundla CLI-en.

Det er viktig å skilje mellom to ting:

1. **Mac app-distribusjon**
   - `.app`
   - installasjon
   - signering
   - ZIP/PKG/CI
2. **Mac app-backend for ekte vindauge**
   - event loop
   - vindauge
   - rendering
   - input
   - Norscode-driven UI

Det første sporet er langt kome.
Det andre er framleis ikkje ferdig som aktiv, verifisert backend.

## Dagens status

Det som finst no:

- [tools/build-macos-app.sh](tools/build-macos-app.sh) lagar ein ekte `.app`
- appen opnar via `Terminal`, ikkje via eige `NSWindow`
- [docs/MAC_APP_OMGANGER.md](MAC_APP_OMGANGER.md) dekkjer distribusjons- og release-sporet
- [docs/FRONTEND_MODES.md](FRONTEND_MODES.md) og [docs/OMGANGER.md](OMGANGER.md) peikar på eit native UI-spor på høgt nivå

Det som ikkje finst ferdig i aktiv flate:

- ferdig macOS-vindusbackend kopla til Norscode-runtime
- verifisert `NSWindow`-/AppKit-opning frå Norscode-app
- ein klar kontrakt for korleis Norscode-UI blir rendra i eit ekte Mac-vindauge

## Sluttdefinisjon

Mac native window-sporet er ferdig når:

- dobbeltklikk på appen opnar eit ekte Mac-vindauge
- vindauget køyrer på ein definert Norscode-backend, ikkje Terminal
- input, rendering og livssyklus går gjennom den same backend-en
- det finst ein minimal, verifiserbar demo-app driven av Norscode

## Omgang 1: Lås backend-retning

Mål:
Vel ein eksplisitt første backend for ekte vindusopning på macOS.

Vi må ikkje prøve å byggje alt på ein gong. Første reelle val er:

1. **WebView-basert appbackend**
   - raskast veg til ekte vindauge
   - Norscode kan drive UI via HTML/component/template-spor
   - Cocoa/AppKit blir mest shell og livssyklus
2. **Native UI/AppKit-backend**
   - meir ambisiøs og “rein”
   - nærare den skisserte native UI-stacken
   - krev mykje meir før første grøne vindauge

Anbefalt primærretning:

1. **WebView først**
2. **native UI/AppKit etterpå**

Kvifor:

- repoet har allereie sterkare frontend-/HTML-/component-spor enn ferdig aktiv native window-backend
- vi får eit ekte Mac-vindauge mykje tidlegare
- vi kan bruke same app-distribusjonslinje som alt er bygd

Ferdig når:

- dokumentasjonen seier eksplisitt kva backend som er første mål
- vi sluttar å omtale dagens Terminal-app som om ekte vindauge allereie er løyst

Status:
Bygd i repoet. Første backend-retning er no låst som WebView først, native UI/AppKit etterpå.

## Omgang 2: Minimal vindushost

Mål:
Bygg ein minimal macOS host som opnar eit ekte app-vindauge.

Første leveranse bør vere:

- eitt vindauge
- fast tittel
- enkel livssyklus
- plass til å vise innhald frå vald backend

Ferdig når:

- `Norscode.app` kan opne eit eige vindauge lokalt
- Terminal ikkje lenger er nødvendig for grunnopning

Status:
Bygd i første praktiske form. [tools/build-macos-window-host.sh](tools/build-macos-window-host.sh) byggjer no ein minimal AppKit + WebView-host som opnar eit ekte macOS-vindauge utan Terminal. Neste steg er å kople denne hosten til Norscode-driven rendering.

## Omgang 3: Kople Norscode-backend til vindauget

Mål:
Kople hosten til ein faktisk Norscode-driven backend.

Dersom første backend er WebView:

- Norscode leverer HTML/UI-innhald
- hosten viser dette i eit ekte Mac-vindauge

Dersom første backend er native UI:

- Norscode må levere widget-/layout-data inn i hosten

Ferdig når:

- vindauget ikkje berre er tomt eller hardkoda
- innhaldet kjem frå ein definert Norscode-kjede

Status:
Bygd i første praktiske form. Vindushosten er no kopla til eit faktisk Norscode-entrypoint via `bin/nc run examples/frontend.no`, fangar HTML-utdata og lastar denne inn i WebView. Koblinga er framleis repo-avhengig, men renderinga kjem no frå Norscode i staden for hardkoda Swift-markup.

## Omgang 4: Input og app-livssyklus

Mål:
Legg på grunnleggjande input og stateflyt.

Målet er:

- klikk
- tastatur
- enkel stateoppdatering
- open/close/focus livssyklus

Ferdig når:

- ei enkel interaktiv demo kan køyrast utan Terminal

Status:
Bygd i første praktiske form. Vindushosten støttar no intern lenkenavigasjon, enkel rute-state, reload via tastatursnarveg og appmeny, og re-rendering av Norscode-innhald for ny sti/query. Dette er framleis WebView-basert og GET-orientert, men gir no ei faktisk enkel input- og livssyklussløyfe utan Terminal.

## Omgang 5: Pakking, signering og CI for ekte GUI-app

Mål:
Oppdatere Mac-app-linja slik at ho pakkar og verifiserer den nye GUI-backenden, ikkje berre Terminal-launcheren.

Målet er:

- `build-macos-app.sh` byggjer GUI-varianten
- signering/verifikasjon dekkjer den nye hosten
- CI byggjer riktig appvariant

Ferdig når:

- release-artefakta representerer den faktiske GUI-appen

Status:
Bygd i første praktiske form. [tools/build-macos-app.sh](tools/build-macos-app.sh) byggjer no GUI-varianten som standard (`--mode gui`), pakkeskriptet brukar GUI-bundlen, og GitHub Actions-workflowen byggjer den same GUI-varianten. Terminal-løypa finst framleis som `--mode terminal`, men er ikkje lenger den primære app-retninga.

## Omgang 6: Sluttstatus og standardretning

Mål:
Avklare kva som no er primær Mac-app-oppleving.

Ferdig når:

- `Terminal app-bundle` er tydeleg sekundær eller vedlikehaldsflate
- `ekte Mac-vindauge` er dokumentert som primær app-oppleving
- repoet har ein kort sluttrapport for kva som er ferdig og kva som framleis er delvis

## Anbefalt rekkefølgje

1. Vel backend: WebView først
2. Lag minimal vindushost
3. Kople Norscode-innhald til hosten
4. Legg på input og livssyklus
5. Flytt pakking/CI over på GUI-varianten
6. Skriv sluttstatus

## Kort vurdering

Den viktigaste sanninga akkurat no er:

- macOS app-sporet er godt bygd som **distribusjon**
- macOS app-sporet er ikkje ferdig som **ekte vindusapp**

Det som manglar er difor ikkje først og fremst meir PKG/ZIP/CI.
Det som manglar er ein ferdig Norscode-backend for sjølve appvindauget.

Den raskaste truverdige vegen vidare er:

1. WebView-basert vindauge først
2. seinare djupare native UI-backend dersom det framleis er målet
