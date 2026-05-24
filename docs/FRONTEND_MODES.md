# Frontend Modes

Norscode kan beskrive frontend på flere nivåer, fra ren HTML til mer deklarativ og reaktiv UI.
Dette dokumentet samler de viktigste modusene i én kontrakt.

## 1. HTML Mode

Vanlig HTML er den laveste og mest direkte formen.

Bruk når:

- du vil skrive markup eksplisitt
- du vil ha full kontroll på strukturen
- du ikke trenger ekstra syntaks eller state

Eksempel:

```no
returner "<section class=\"card\"><h2>Velkommen</h2></section>"
```

## 2. Template Mode

Template mode er HTML med variabler, typisk `{{ }}`-innsetting av tekst.

Bruk når:

- du vil holde markup lesbar
- du bare trenger enkel innsetting av data
- du vil unngå mye konkatenasjon

Eksempel:

```no
side:
    tittel "{{navn}}"
    tekst "{{melding}}"
```

## 3. Native UI Mode

Native UI mode er norsk, blokk-basert UI-syntax som leses mer som struktur enn kode.

Bruk når:

- du vil at UI skal ligne naturlig domenetekst
- du vil skrive deklarative visninger med mindre støy
- du vil bygge et tydelig språk rundt layout og komponenter

Eksempel:

```no
side:
    hero:
        tittel "Velkommen til Norscode"
        tekst "Bygg UI direkte med norsk, blokk-basert syntaks."
    tabs:
        tittel "Frontend-moduser"
        tab:
            tittel "HTML mode"
            tekst "Vanlig HTML"
        tab:
            tittel "Template mode"
            tekst "{{ }} variabler"
        tab:
            tittel "Native UI mode"
            tekst "Norsk UI-syntax"
        tab:
            tittel "Component mode"
            tekst "Gjenbrukbare komponenter"
        tab:
            tittel "Reactive mode"
            tekst "Automatisk UI-oppdatering"
    accordion:
        tittel "Vanlige spørsmål"
        seksjon:
            tittel "Hva er Native UI?"
            tekst "En norsk, blokk-basert måte å beskrive UI på."
        seksjon:
            tittel "Når bør jeg bruke det?"
            tekst "Når du vil bygge server-rendret UI med tydelig struktur."
        seksjon:
            tittel "Er det interaktivt?"
            tekst "Ja, men interaktivitet kan bygges stegvis på toppen."
    badge:
        tekst "Ny"
    alert:
        tittel "Merk"
        tekst "Dette er en alert"
    empty:
        tittel "Ingen elementer"
        tekst "Det finnes ingenting her ennå."
    loading:
        tekst "Laster inn data..."
    form:
        felt:
            label:
                tekst "Navn"
                for_id "navn"
            input:
                type "text"
                navn "navn"
                placeholder "Skriv navnet ditt"
        felt:
            label:
                tekst "E-post"
                for_id "epost"
            input:
                type "email"
                navn "epost"
                placeholder "navn@eksempel.no"
        felt:
            label:
                tekst "Melding"
                for_id "melding"
            textarea:
                navn "melding"
                rader "4"
                placeholder "Skriv en kort melding"
                tekst "Hei, jeg vil vite mer."
        felt:
            label:
                tekst "Kategori"
                for_id "kategori"
            select:
                navn "kategori"
                option:
                    tekst "Velg en kategori"
                    verdi ""
                option:
                    tekst "Generelt"
                    verdi "generelt"
                option:
                    tekst "Teknisk"
                    verdi "teknisk"
        felt:
            checkbox:
                navn "samtykke"
                tekst "Jeg vil motta oppdateringer"
                checked "checked"
        felt:
            radio:
                navn "nyhetsbrev"
                verdi "ukentlig"
                tekst "Ukentlig"
        felt:
            radio:
                navn "nyhetsbrev"
                verdi "månedlig"
                tekst "Månedlig"
    actions:
        knapp "Send"
        knapp "Avbryt"
    nav:
        lenke "HTML mode" /
        lenke "Component mode" /component
    kort:
        tittel "Velkommen"
        tekst "Norscode App"
    liste:
        element "HTML mode"
        element "Component mode"
    footer:
        tittel "Norscode"
        tekst "Bygd med Native UI."
```

Implementasjon:

- `norcode ui-render examples/native_ui.nui`
- CLI-kommandoen starter [`std/nativeui.no`](/Users/jansteinar/Projects/Norscode/std/nativeui.no) via `bin/nc run`
- renderer- og parserlogikken ligger i [`std/nativeui.no`](/Users/jansteinar/Projects/Norscode/std/nativeui.no)
- Python brukes bare som CLI-launcher, ikke som selve UI-implementasjonen

## 4. Component Mode

Component mode er gjenbrukbare UI-byggeklosser med eksplisitte parametere.

Bruk når:

- du vil dele markup mellom sider
- du vil ha små, testbare visninger
- du vil holde layout og innhold adskilt

Eksempel:

```no
funksjon kort(tittel: tekst, tekst_innhold: tekst) -> tekst {
    returner html.partial_card("", tittel, html.p("", html.escape(tekst_innhold)))
}
```

## 5. Reactive Mode

Reactive mode er når UI oppdateres automatisk når state endrer seg.

Bruk når:

- du trenger dynamisk oppførsel i browseren
- du vil slippe manuell rerendering av hele siden
- du vil ha komponenter som reagerer på state og events

Eksempel:

```no
state.teller = state.teller + 1
ui.oppdater()
```

## Anbefalt rekkefølge

For Norscode er dette den naturlige progresjonen:

1. HTML mode
2. Template mode
3. Component mode
4. Native UI mode
5. Reactive mode

## Praktisk regel

Ikke start med reaktivitet hvis du bare trenger server-renderte sider.
Bruk det enkleste nivået som passer problemet.

Se også:

- [docs/FRONTEND_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_MODEL.md)
- [docs/FRONTEND_COMPONENT_MODEL.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_COMPONENT_MODEL.md)
- [docs/FRONTEND_LAYOUT_CONTRACT.md](/Users/jansteinar/Projects/Norscode/docs/FRONTEND_LAYOUT_CONTRACT.md)
