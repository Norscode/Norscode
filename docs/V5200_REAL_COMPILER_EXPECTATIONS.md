# V5200 real compiler expectations

## Mål

Dette steget definerer kva vi forventar å sjå frå probe-settet i `*_real`-kjeda.

Fokus er ikkje full optimalisering eller pen output. Fokus er å bevise at rett
bytecode faktisk blir bygd.

## Forventa signal per probe

### `list_probe_real.no`

Må vise:
- `PUSH_CONST`
- `BUILD_LIST`

### `empty_list_probe_real.no`

Må vise:
- `BUILD_LIST`

### `map_probe_real.no`

Må vise:
- `PUSH_CONST`
- `BUILD_MAP`

### `index_probe_real.no`

Må vise:
- `LOAD_VAR`
- `INDEX_GET`

### `assignment_list_probe_real.no`

Må vise:
- `PUSH_CONST`
- `BUILD_LIST`
- `STORE_VAR`

### `assignment_map_probe_real.no`

Må vise:
- `PUSH_CONST`
- `BUILD_MAP`
- `STORE_VAR`

### `assignment_index_probe_real.no`

Må vise:
- `LOAD_VAR`
- `INDEX_GET`
- `STORE_VAR`

## Viktig presisering

Desse forventningane beviser berre `*_real`-referansekjeda.

Dei beviser ikkje at:
- aktiv `dist/norscode_native` er fiksa
- bootstrap/generated-C compile-løypa er fiksa
- `v4600-v4800` er grøn i den gamle lina

## Neste bruk

Når desse signala er stabile, kan vi bruke dei som fasit når vi reparerer den
aktive bootstrap-compileren.
