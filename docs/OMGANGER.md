# Omganger

Dette dokumentet samler status for Norscode-omgangene og forklarer hva hver fase har introdusert i runtime-retningen.

## Kort status

Norscode har passert fra generell runtime-arkitektur til begynnende execution-runtime-design.

Siste dokumenterte store milepæl: Omgang 191.

Siste hovedretning:

- semantic transform
- evaluator operations
- control-flow
- persistence og recovery
- transactional runtime
- validation og governance
- context/session/scope/frame/stack
- push/pop
- call/return

## Fase 1: Semantic transform

Omgangene rundt 156 til 159 introduserte den første primitive transform-modellen.

Kjerne:

```text
state + input + operation + rule -> updated + output
```

Viktig betydning:

- første state-transform-retning
- første primitive evaluator-kontrakt
- første operation/rule-modell

## Fase 2: Primitive semantic operations

Omgangene rundt 160 til 164 introduserte konkrete execution-begreper.

Kjernebegreper:

- assign
- compare
- branch
- select
- merge

Viktig betydning:

- første konkrete state-binding-retning
- første comparison/conditional-semantikk
- første primitive control-flow
- første branch convergence via merge

## Fase 3: Evaluator cycles

Omgangene rundt 165 til 170 introduserte evaluator-loop og recovery-retning.

Kjernebegreper:

- evaluate
- iterate
- continue
- repeat
- restart
- resume

Viktig betydning:

- første evaluator-cycle-model
- første persistent execution-loop
- første restart/resume-retning

## Fase 4: Persistence og transactional runtime

Omgangene rundt 171 til 176 introduserte persistent state, commit/rollback og validation.

Kjernebegreper:

- checkpoint
- store
- load
- commit
- rollback
- validate

Viktig betydning:

- runtime snapshots
- durable state
- restore/recovery
- transactional execution
- validated transitions

## Fase 5: Safety, governance og access control

Omgangene rundt 177 til 183 introduserte styringslag for execution.

Kjernebegreper:

- guard
- assert
- constraint
- policy
- capability
- permission
- role

Viktig betydning:

- guarded execution
- assertions og invariants
- semantic constraints
- policy-governed runtime
- capability og permission-modell
- role-based execution authority

## Fase 6: Runtime memory hierarchy

Omgangene rundt 184 til 188 introduserte execution-domener og memory-struktur.

Kjernebegreper:

- context
- session
- scope
- frame
- stack

Viktig betydning:

- isolated execution contexts
- persistent sessions
- local scopes
- evaluator frames
- call stacks

## Fase 7: Mutable stack og callable runtime

Omgangene 189 til 191 introduserte stack-mutasjon og call/return.

Kjernebegreper:

- push
- pop
- call
- return

Viktig betydning:

- stack growth
- stack unwinding
- nested evaluator invocation
- return propagation
- første primitive callable execution-runtime

## Nåværende hovedmodell

Et forenklet bilde etter Omgang 191:

```text
state
+ context
+ session
+ scope
+ frame
+ stack
+ push/pop
+ call/return
+ checkpoint/store/load
+ validate/guard/assert/constraint/policy
+ capability/permission/role
+ commit/rollback
+ input/operation/evaluate
+ iterate/continue/repeat/restart/resume
+ compare/branch/select/merge/assign/rule
-> updated + output
```

## Python-fri status

Norscode er nå tydelig på vei mot Python-uavhengig runtime-arkitektur, men er ikke en full Python-erstatning ennå.

Sterkt utviklet:

- runtime-arkitektur
- semantic execution design
- persistence/recovery
- governance
- scoped runtime
- call-stack design

Mangler fortsatt:

- ekte value/data-semantikk
- bindings og symboloppslag
- expression evaluation
- arithmetic og boolean computation
- AST/parser
- faktisk kjørende evaluator

## Neste naturlige fase

Neste store fase bør være value/binding/expression-semantikk.

Foreslåtte kommende byggesteg:

1. value
2. symbol
3. bind
4. lookup
5. resolve
6. expression
7. compute
8. boolean
9. integer
10. function

Den neste store milepælen blir første ekte runtime-value-transformasjon, for eksempel:

```text
assign(x, value) -> updated state
```

eller:

```text
1 + 1 -> 2
```

Når dette skjer, begynner Norscode å bevege seg fra execution-arkitektur til faktisk computation.

---

## Fase 8: Typesystem (z253–z262)

Omgang 253–262 introduserte komplett typesystem-semantikk.

Siste dokumenterte store milepæl: Omgang 262.

Kjerne:

```text
source -> parse -> type_check -> typed_ast -> lower -> codegen
```

### z253 — Typesystem-primitiver og typerepresentasjon

- `type_repr`, `type_id`, `type_kind`
- Primitive typer: `type_int`, `type_float`, `type_bool`, `type_text`, `type_unit`, `type_never`
- Sammensatte typer: `type_list`, `type_map`, `type_struct`, `type_union`, `type_function`, `type_optional`
- `type_registry`, `type_intern`, `type_canon`
- `type_eq`, `type_compat`, `type_coerce`

### z254 — Typeinferens og unifikasjon

- `infer_expr`, `infer_func`, `infer_call`, `infer_literal`
- `unify`, `unify_pair`, `substitution`, `apply_subst`, `compose_subst`
- `occurs_check` — forhindrer sirkulære typer
- `generalize`, `instantiate`, `scheme`, `free_vars`
- `type_mismatch`, `ambiguous_type`

### z255 — Struct-typer og felttilgang

- `struct_def`, `struct_field`, `field_index`, `field_offset`
- `struct_constructor`, `field_init`, `missing_field`, `extra_field`
- `field_access`, `field_path`, `nested_access`
- `struct_layout`, `layout_size`, `layout_align`
- `struct_method`, `method_receiver`, `method_dispatch`

### z256 — Union-typer og utømmende mønstermatching

- `union_def`, `variant`, `variant_tag`, `variant_payload`
- `match_expr`, `match_arm`, `match_pattern`, `match_guard`
- `pattern_variant`, `pattern_literal`, `pattern_bind`, `pattern_wildcard`
- `exhaustiveness`, `covered_variants`, `uncovered_variant`
- `non_exhaustive`, `redundant_arm`

### z257 — Generics og parametrisk polymorfisme

- `generic_def`, `param_list`, `param_bound`, `param_default`
- `instantiation`, `type_arg`, `inst_key`, `inst_cache`
- `bound_check`, `bound_violation`, `where_clause`
- `monomorphize`, `mono_instance`, `mono_cache`
- `infer_type_args`, `arity_mismatch`

### z258 — Typekontroll-pipeline og semantisk validering

- `check_pipeline`, `check_func`, `check_block`, `check_expr`
- `check_assign`, `check_return`, `check_call`, `check_field`
- `return_type_stack`, `missing_return`, `dead_code_after_return`
- `narrowing_pass`, `branch_types`, `join_types`
- `error_accumulator`, `error_recovery`, `skip_subtree`

### z259 — Typefeil-rapportering og diagnostikk

- `diagnostic`, `diagnostic_level`, `diagnostic_code`, `diagnostic_span`
- `primary_span`, `secondary_span`, `span_label`
- `suggestion`, `suggestion_insert`, `suggestion_replace`, `suggestion_delete`
- `render_source_window`, `format_diagnostic`
- `error_count`, `warning_count`

### z260 — Typemetadata og runtime-refleksjon

- `type_metadata`, `meta_fields`, `meta_variants`, `meta_methods`
- `reflect_value`, `type_of`, `fields_of`, `variants_of`
- `dynamic_type`, `type_tag`, `is_type`, `as_type`, `cast_dynamic`
- `serialize_type`, `type_to_json`, `type_schema`

### z261 — Subtyping, koersjon og implisitte konverteringer

- `subtype_rel`, `subtype_check`, `subtype_chain`
- `implicit_coerce`, `explicit_coerce`, `coerce_insert`
- `widen_int`, `int_to_float`, `lift_to_optional`, `lift_to_union`
- `numeric_promotion`, `promote_operands`, `common_numeric_type`
- `no_implicit_coerce`, `coerce_loss_of_precision`

### z262 — Typesystem-integrasjon med compiler-pipeline

- `typed_ast`, `typed_node`, `typed_expr`, `typed_module`
- `type_lowering`, `erase_types`, `emit_type_metadata`
- `typed_ir_func`, `typed_ir_instr`, `ir_type_annotation`
- `type_driven_opt`, `type_guided_inline`, `type_guided_specialize`
- `incremental_typecheck`, `module_type_signature`, `signature_hash`
- `type_export`, `type_import`, `cross_module_type`

### Nåværende modell etter Omgang 262

```text
source_unit
-> parse_stage -> ast
-> type_check_stage
   + type_env + type_repr + type_registry
   + infer (HM-delmengde) + unify + substitution
   + check_pipeline (stmt/expr/func/block)
   + narrowing_pass + branch_types + join_types
   + exhaustiveness (union/match)
   + bound_check (generics)
   + subtype_check + coerce_insert
-> typed_ast
-> type_lowering -> typed_ir (med type_id på hver verdi)
-> erase_types eller emit_type_metadata (per backend)
-> lower_stage + codegen_stage -> output
```

### Neste naturlige fase etter z262

Neste store omgang bør være **WASM-backend** (z263+):

1. WASM binary format og modulstruktur
2. WASM instruksjonskoding
3. WASM lineær minnemodell
4. WASM funksjonskall og tabeller
5. WASM JS-interop og host bindings
6. WASM kjøretid og validering
7. WASM kildekart
8. WASI system interface

---

## Fase 9: WASM-backend (z263–z272)

Omgang 263–272 introduserte komplett WASM-backend fra IR til distribuerbar binær.

Siste dokumenterte store milepæl: Omgang 272.

Kjerne:

```text
typed_ir -> wasm_backend -> lower_to_wasm -> emit_driver -> wasm_bytes
```

### z263 — WASM binary format og modulstruktur

- `wasm_module`, `wasm_section`, `section_id`, `section_order`
- Alle seksjoner: `type_section`, `import_section`, `function_section`, `code_section`, `data_section` m.fl.
- `func_type`, `type_index`, `func_index`
- `wasm_import`, `wasm_export`, `wasm_global`
- `magic_bytes`, `leb128_encode`, `wasm_buffer`

### z264 — WASM instruksjonssett og bytekode-koding

- Aritmetikk: `add_i32/i64/f64`, `sub`, `mul`, `div`, `rem`
- Sammenligning: `eq_i32`, `lt_i32_s`, `eqz_i32`
- Lokal-/global-tilgang: `local_get`, `local_set`, `local_tee`, `global_get`, `global_set`
- Minneaksess: `load_i32/f64`, `store_i32/f64`, `load_offset`, `load_align`
- Kontrollflyt: `block`, `loop_instr`, `if_instr`, `br`, `br_if`, `br_table`
- Kall: `call`, `call_indirect`, `return_instr`
- Konverteringer: `i32_wrap_i64`, `i64_extend_i32_s`, `f64_convert_i32_s`

### z265 — WASM lineær minnemodell og heap-layout

- `linear_memory`, `page_size`, `initial_pages`, `memory_grow`
- Regioner: `static_region`, `stack_region`, `heap_region`, `data_region`
- `bump_alloc`, `alloc_addr`, `alloc_align`
- Objektlayout: `string_layout`, `list_layout`, `struct_layout`
- `rc_header`, `rc_increment`, `rc_decrement`, `rc_free`
- `data_segment`, `segment_offset`, `segment_bytes`

### z266 — WASM funksjonskall, kallekonvensjon og closures

- `wasm_func`, `func_body`, `func_locals`, `func_prologue`, `func_epilogue`
- `call_conv`, `arg_passing`, `result_passing`, `multi_value_return`
- `closure`, `closure_ptr`, `closure_env`, `closure_alloc`, `closure_call`
- `trampoline`, `indirect_call_table`, `table_slot`
- `builtin_func`, `builtin_dispatch`, `runtime_import`
- `tail_call_opt`, `shadow_stack`, `shadow_frame`

### z267 — WASM JS-interop og host bindings

- `host_env`, `host_func`, `host_import`, `host_export`
- `js_value`, `extern_ref`, `extern_ref_table`
- `marshal_to_js`, `marshal_from_js`, `coerce_js_string`, `coerce_js_number`
- `wasm_bindgen`, `export_attr`, `ts_decl`
- `dom_api`, `fetch_api`, `promise`, `async_bridge`

### z268 — WASM validering og modulverifisering

- `validator`, `validate_module`, `validate_func`, `validate_instr`
- `val_stack`, `val_push`, `val_pop`, `ctrl_stack`, `ctrl_frame`
- `type_check_instr`, `reachability`, `polymorphic_stack`
- `stack_underflow`, `type_mismatch_val`, `undefined_func`
- `validation_result`, `error_location`

### z269 — WASM kjøretid og eksekusjonsmotor

- `wasm_instance`, `instantiate`, `invoke_export`, `invoke_func`
- `value_stack`, `wasm_value`, `val_i32/i64/f32/f64/ref`
- `dispatch_instr`, `exec_arith`, `exec_memory_load/store`, `exec_call`
- `trap`, `trap_memory_oob`, `trap_div_zero`, `trap_indirect_type_mismatch`
- `fuel`, `fuel_limit`, `fuel_check`, `fuel_exhausted`
- `jit_tier`, `jit_threshold`, `jit_compile`, `jit_cache`

### z270 — WASM kildekart og debug-informasjon

- `source_map`, `source_map_mappings`, `mapping`, `vlq_encode`, `vlq_segment`
- `instr_location`, `instr_offset`, `source_line`, `source_col`
- `wasm_name_section`, `func_name`, `local_name`, `name_map`
- `debug_info_section`, `die_subprogram`, `die_variable`, `die_type_ref`
- `breakpoint_table`, `bp_entry`, `bp_instr_offset`
- `sourcemap_emit`, `sourcemap_encode`, `sourcemap_json`

### z271 — WASI systemgrensesnitt og plattformintegrasjon

- `wasi_snapshot_preview1`, `wasi_import`, `wasi_errno`
- `fd_read`, `fd_write`, `fd_seek`, `fd_close`, `fd_stat`
- `path_open`, `path_stat`, `path_unlink_file`, `path_rename`
- `args_get`, `args_sizes_get`, `environ_get`, `environ_sizes_get`
- `clock_time_get`, `clock_realtime`, `clock_monotonic`
- `proc_exit`, `random_get`, `poll_oneoff`
- `preopened_dir`, `iovec`, `wasi_rights`, `wasi_filetype`

### z272 — WASM backend-pipeline og emit-driver

- `wasm_backend`, `backend_session`, `backend_options`
- `lower_to_wasm`, `lower_func`, `lower_block`, `lower_instr`
- `local_map`, `local_alloc`, `type_map`, `ir_type_to_wasm`
- `const_pool`, `emit_const_pool`
- `wasm_optimizer`, `opt_const_fold`, `opt_local_reduce`
- `emit_driver`: alle seksjons-emittere i riktig rekkefølge
- `output_artifact`: `wasm_file`, `ts_decl_file`, `sourcemap_file`, `output_manifest`

### Nåværende modell etter Omgang 272

```text
typed_ir_module
-> wasm_backend (backend_session + backend_options)
-> lower_to_wasm:
   for each typed_ir_func:
     local_map + local_alloc
     lower_func -> lower_block -> lower_instr*
     type_map (ir_type -> wasm val type)
     closure_alloc + shadow_stack (ved behov)
-> const_pool (strenger + type_metadata)
-> wasm_optimizer (const_fold, local_reduce, dead_instr)
-> emit_driver:
   type_section -> import_section -> function_section
   -> memory_section -> export_section -> code_section
   -> data_section (const_pool) -> name_section -> sourcemap_section
-> wasm_bytes -> wasm_file
-> ts_decl_file + sourcemap_file
-> wasi_imports (fd_read/write, path_open, clock, args, proc_exit)
-> output_manifest -> build_cache
```

### Neste naturlige fase etter z272

Neste store omgang bør være **LSP og IDE-støtte** (z273+):

1. JSON-RPC 2.0 protokoll og meldingsloop
2. Inkrementell parsing og dokument-synkronisering
3. Diagnostikk i sanntid (syntaks- og typefeil)
4. Hover (type og dokumentasjon ved cursor)
5. Go-to-definition
6. Autocomplete (variabel, funksjon, felt)
7. Rename symbol
8. Format on save
