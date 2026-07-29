# V5600 Bootstrap migration checklist

## Formål
Dette steget pakkar den aktive compile-løypa inn i faste reproduksjonsfiler og forventningssjekkar.

No har vi to parallelle spor:
- `*_real` som referansefasit
- bootstrap/generated-C som faktisk må rettast

## Filer i denne fasen

Kjeldeprober:
- `tests/fixtures/bootstrap_v5600/list_literal_v5600.no`
- `tests/fixtures/bootstrap_v5600/empty_list_v5600.no`
- `tests/fixtures/bootstrap_v5600/map_literal_v5600.no`
- `tests/fixtures/bootstrap_v5600/index_expr_v5600.no`
- `tests/fixtures/bootstrap_v5600/assignment_list_v5600.no`
- `tests/fixtures/bootstrap_v5600/assignment_map_v5600.no`
- `tests/fixtures/bootstrap_v5600/assignment_index_v5600.no`

Verktøy:
- `tools/run_bootstrap_literal_probes_v5600.no`
- `tools/check_bootstrap_literal_expectations_v5600.no`

## Grønn definisjon
Bootstrap-compiler er grøn for denne blokka når desse påstandane blir sanne i generert NCB/JSON:
- list literal gir `BUILD_LIST`
- tom liste gir `BUILD_LIST`
- map literal gir `BUILD_MAP`
- indeksuttrykk gir `INDEX_GET`
- assignment med liste/map/indeks bevarer verdiuttrykket før lagring

## Arbeidsrekkefølgje vidare
1. Kople aktiv compile-løype til rett lowering/emitter for liste.
2. Rerun `list_literal_v5600.no` og `empty_list_v5600.no`.
3. Kople map-lowering/emitter.
4. Rerun `map_literal_v5600.no`.
5. Kople indeks-lowering/emitter.
6. Rerun `index_expr_v5600.no`.
7. Lukke assignment etter at verdiuttrykka er stabile.
8. Først etter dette: rerun `v4800` og `v4600`.

## Viktig
Dette steget gjer ikkje påstand om at bootstrap-compileren er frisk. Det gjer berre resten av reparasjonen målbar og repeterbar.
