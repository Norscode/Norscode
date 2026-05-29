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

---

## Fase 10: LSP og IDE-støtte (z273–z282)

Omgang 273–282 introduserte komplett language server og IDE-integrasjon.

Siste dokumenterte store milepæl: Omgang 282.

Kjerne:

```text
nc lsp -> message_loop -> dispatch_message -> LSP capabilities -> editor
```

### z273 — JSON-RPC 2.0 protokoll og meldingsloop

- `jsonrpc_message`, `jsonrpc_request`, `jsonrpc_response`, `jsonrpc_notification`
- `lsp_transport`, `stdio_transport`, `read_message`, `write_message`, `message_framing`
- `content_length`, `header_separator`, `body_bytes`
- `message_loop`, `dispatch_message`, `handle_request`, `handle_notification`
- `leb128_encode`, `utf8_encode`, `cancel_request`, `progress_token`

### z274 — Dokumentsynkronisering og inkrementell parsing

- `document_store`, `document_uri`, `document_version`, `document_text`
- `did_open`, `did_change`, `did_close`, `did_save`
- `text_edit`, `apply_edit`, `line_index`, `build_line_index`
- `offset_to_position`, `position_to_offset`, `utf16_offset`
- `reparse_debounce`, `document_snapshot`, `doc_cache`

### z275 — Diagnostikk og sanntids-feilrapportering

- `diagnostic`, `diagnostic_severity`, `diagnostic_code`, `diagnostic_source`
- `publish_diagnostics`, `push_diagnostics`, `pull_diagnostics`
- `from_parse_errors`, `from_type_errors`, `from_lint_rules`
- `merge_diagnostics`, `sort_diagnostics`, `cap_diagnostics`
- `lint_rule`, `lint_fix`, `tag_unnecessary`, `tag_deprecated`, `version_guard`

### z276 — Hover-provider og dokumentasjon

- `hover_request`, `hover_response`, `hover_contents`, `hover_range`
- `markup_markdown`, `compute_hover`, `node_at_position`, `enclosing_node`
- `type_display`, `format_func_sig`, `format_struct_type`, `format_union_type`
- `doc_extractor`, `doc_paragraph`, `doc_param`, `doc_return`, `doc_example`
- `hover_sections`, `section_signature`, `section_doc`, `section_defined_in`

### z277 — Go-to-definition og symboloppløsning

- `definition_request`, `definition_location`, `definition_link`
- `declaration_request`, `type_definition_request`, `references_request`
- `resolve_at_position`, `resolve_ident`, `resolve_field`, `resolve_method`
- `definition_index`, `index_by_position`, `cross_file_resolve`
- `workspace_symbol`, `fuzzy_match`, `symbol_result_list`

### z278 — Autocomplete og komplettering

- `completion_request`, `completion_list`, `completion_item`, `item_kind`
- `scope_completions`, `member_completions`, `keyword_completions`, `import_completions`
- `snippet_completions`, `insert_text_format_snippet`, `snippet_tab_stop`
- `prefix_at_position`, `fuzzy_filter_items`, `sort_completion_items`
- `resolve_completion`, `trigger_dot`, `trigger_open_paren`

### z279 — Rename, code actions og refaktorering

- `rename_request`, `prepare_rename`, `validate_new_name`, `rename_all_references`
- `workspace_edit`, `document_changes`, `text_document_edit`, `annotated_edit`
- `code_action_request`, `code_action`, `action_kind`, `action_is_preferred`
- `quickfix_provider`, `quickfix_insert_import`, `quickfix_fix_type_coerce`
- `extract_function`, `extract_variable`, `organize_imports`

### z280 — Formattering, signaturhjelp og semantiske tokens

- `format_request`, `formatter`, `format_edits`, `indent_level`, `trailing_comma`
- `on_type_format_request`, `range_format_request`
- `signature_help_request`, `signature_information`, `parameter_information`, `active_parameter`
- `inlay_hint`, `hint_kind_type`, `hint_kind_parameter`, `hint_text_edits`
- `semantic_tokens_full`, `token_data`, `token_type_*`, `semantic_tokens_legend`

### z281 — Server-livssyklus og capability-forhandling

- `lsp_server`, `server_state`: `uninitialized → initializing → running → shutdown → exited`
- `initialize_request`, `initialize_params`, `capabilities_client`, `capabilities_server`
- `initialized_notification`, `shutdown_request`, `exit_notification`
- `client_capability_check`, `supports_markdown`, `supports_snippet`, `supports_prepare_rename`
- `work_done_progress_create`, `progress_notification`

### z282 — LSP-klientintegrasjon og IDE-utvidelser

- `vscode_extension`, `extension_id`, `activation_events`, `contribute_languages`
- `textmate_grammar`, `grammar_scope_name`, `contribute_commands`, `contribute_configuration`
- `neovim_config`, `lspconfig_setup`, `lspconfig_on_attach`, `lspconfig_capabilities`
- `helix_config`, `helix_language_server`, `helix_formatter`
- `keybind_definition/hover/references/rename/code_action/format`
- `vsix_package`, `marketplace_publish`

### Nåværende modell etter Omgang 282

```text
nc lsp (stdio_transport)
-> message_loop + dispatch_message
-> initialize_request:
   capabilities_server (hover, completion, definition, references,
   rename, format, code_actions, semantic_tokens, inlay_hints,
   diagnostics push+pull, signature_help, workspace_symbol)
-> state_running:
   did_open/change/close -> document_store + reparse_debounce
   -> document_snapshot (ast + tokens + parse_errors)
   -> publish_diagnostics (parse + type + lint)
   textDocument/hover      -> node_at_position -> type_display + doc_extractor
   textDocument/completion -> scope/member/keyword/snippet completions
   textDocument/definition -> resolve_at_position -> definition_index
   textDocument/references -> find_all_refs -> reference_list
   textDocument/rename     -> rename_all_references -> workspace_edit
   textDocument/formatting -> formatter(ast) -> format_edits
   textDocument/codeAction -> quickfix_provider + refactor actions
   textDocument/semanticTokens/full -> token classification -> token_data
   textDocument/inlayHint  -> typed_ast walk -> hint_kind_type + hint_kind_parameter
-> VS Code: vscode_extension + textmate_grammar + contribute_*
-> Neovim: lspconfig_setup + lspconfig_on_attach
-> Helix: helix_config entry
```

### Neste naturlige fase etter z282

Neste store omgang bør være **pakkebehandler og registry** (z283+):

1. `norcode.toml` pakkeformat og feltmodell
2. Versjonering, SemVer og kompatibilitetsregler
3. Avhengighetsgraf og oppløsningsalgoritme
4. Låsefil og reproduserbare bygg
5. Registry-protokoll (HTTP, manifest, SHA256)
6. `nc hent` — nedlasting og caching
7. `nc publiser` — pakking, signering og opplasting
8. Privat registry og speilservere

---

## Fase 11: Pakkebehandler og registry (z283–z292)

Omgang 283–292 introduserte komplett pakkebehandler fra manifest til distribusjon.

Siste dokumenterte store milepæl: Omgang 292.

Kjerne:

```text
norcode.toml -> resolver -> lockfile -> fetch -> build -> publish -> registry
```

### z283 — norcode.toml manifestformat og prosjektmodell

- `manifest`, `project_section`: navn, versjon, beskrivelse, forfattere, lisens
- `dependencies_section`, `dependency_entry`, `dep_version_req`, `dep_path`, `dep_optional`
- `dev_dependencies_section`, `build_dependencies_section`
- `features_section`, `feature_name`, `feature_deps`, `default_features`
- `workspace_section`, `workspace_members`
- `registry_section`, `toml_parse`, `toml_table`

### z284 — SemVer versjonering og kompatibilitetsregler

- `version`: `ver_major.ver_minor.ver_patch[-pre][+build]`
- `version_req`, `req_op`: `op_caret`, `op_tilde`, `op_wildcard`, `op_exact`, `op_greater_eq`
- `caret_rule`: ^1.2.3 → >=1.2.3, <2.0.0; ^0.2.3 → >=0.2.3, <0.3.0
- `req_matches`, `filter_by_req`, `select_best`, `latest_compatible`
- `bump_major`, `bump_minor`, `bump_patch`, `next_version`

### z285 — Avhengighetsgraf og oppløsningsalgoritme

- `dep_graph`, `dep_node`, `dep_edge`, `resolver`
- `candidate_set`, `activate`, `activation_stack`
- `conflict`, `backtrack`, `backtrack_point`, `backtrack_limit`
- `feature_activation`, `propagate_features`, `optional_dep_gate`
- `strategy_newest`, `strategy_locked`, `cycle_check`, `multiple_versions`

### z286 — Låsefil og reproduserbare bygg

- `lockfile`, `locked_package`, `locked_version`, `locked_checksum`, `locked_source`
- `source_kind`: registry, git (med commit-pin), path
- `checksum_verify`, `checksum_mismatch`
- `lock_generate`, `lock_diff`, `lock_check`, `lock_stale`
- `reproducible_build`, `build_input_hash`, `deterministic_output`, `build_id`
- `update_conservative`, `update_minimal`, `lock_audit`, `audit_advisory`

### z287 — Registry-protokoll og pakkeindeks

- `registry`, `registry_url`, `registry_auth`, `auth_token`, `auth_bearer`
- `index`, `index_entry`, `index_version_list`, `index_etag`, `index_refresh`
- `package_metadata`, `meta_checksum`, `meta_yanked`
- `http_client`, `http_retry`, `http_user_agent`
- `search_query`, `search_results`, `result_score`
- `yank`, `yank_reason`

### z288 — nc hent — nedlasting, verifisering og lokal cache

- `fetch_command`, `fetch_all`, `fetch_offline`
- `download_manager`, `download_job`, `download_progress`, `parallel_download`
- `archive_tar_gz`, `extract_archive`, `extract_strip_prefix`
- `package_cache`, `cache_dir`, `cache_key`, `cache_lock`, `cache_evict`
- `verify_download`, `verify_signature`, `ed25519_verify`
- `registry_token`, `token_store`

### z289 — nc publiser — pakking, signering og opplasting

- `publish_command`, `publish_dry_run`, `publish_allow_dirty`
- `package_builder`, `include_glob`, `exclude_glob`, `build_source_list`
- `archive_builder`, `archive_checksum`
- `package_signer`, `ed25519_sign`, `signing_key`
- `publish_metadata`, `publish_readme_content`, `publish_links`
- `publish_form`, `multipart_boundary`
- `pre_publish_check`: versjon ny, lisens, readme, ren git, manifest gyldig
- `version_bump_command`, `bump_and_tag`, `git_tag_push`

### z290 — Privat registry og speilservere

- `registry_server`, `server_config`, `server_tls`
- `storage_filesystem`, `storage_s3`, `storage_gcs`
- `registry_db`, `db_packages`, `db_versions`, `db_tokens`, `db_owners`
- `auth_system`, `api_token`, `scope_publish`, `scope_yank`
- `owner_record`, `owner_add`, `owner_remove`
- `mirror`, `mirror_upstream`, `mirror_sync`, `proxy_fetch_upstream`
- `rate_limiter`, `rate_exceeded`

### z291 — Workspace og multi-pakke-prosjekter

- `workspace`, `workspace_root`, `workspace_members`, `unified_lockfile`
- `cross_member_dep`, `internal_dep_path`
- `workspace_resolver`, `build_order`, `build_layer`, `parallel_build`
- `workspace_inheritance`, `inherit_version`, `workspace_dep`
- `patch_section`, `patch_with_path`, `patch_with_git`
- `virtual_manifest`, `private_package`, `publish_order`

### z292 — Pakkebehandler-CLI og byggesystem-integrasjon

- `pm_cli`: `init`, `add`, `remove`, `update`, `fetch`, `bygg`, `test`, `run`, `publiser`
- `cmd_search`, `cmd_audit`, `cmd_clean`, `cmd_version_bump`, `cmd_login`, `cmd_yank`
- `init_template`: `template_lib`, `template_bin`, `template_wasm`
- `build_script`, `rerun_if_changed`, `build_script_output`
- `build_cache`, `cached_artifact`, `artifact_key`, `artifact_invalidate`
- `output_json` for verktøy-integrasjon

### Nåværende modell etter Omgang 292

```text
norcode.toml (manifest)
-> toml_parse -> project_section + dependencies_section + features_section
-> resolver: candidate_set + backtracking SAT + feature_propagation
-> unified_lockfile (alle workspace-members)
-> fetch_all: parallel_download + verify_checksum + package_cache
-> build_order (topologisk) -> parallel_build per lag
   build_cache: artifact_key -> cache_hit (hopp over) eller compile
   build_script (om den finnes) -> rerun_if_changed
-> nc test -> test_runner -> test_result_aggregate
-> nc publiser: pre_publish_check -> archive_builder -> ed25519_sign
   -> multipart HTTP POST -> registry
-> registry_server: storage (FS/S3/GCS) + db + auth + mirror + rate_limit
-> workspace: unified_lockfile + workspace_dep + patch_section
```

### Neste naturlige fase etter z292

Neste store omgang bør være **CI/CD og release-automatisering** (z293+):

1. GitHub Actions workflow for Norscode-prosjekter
2. Automatisk bygg og test på push og PR
3. Release-pipeline utløst av versjonstagg
4. Plattformkryssbygg (macOS, Linux, Windows, WASM)
5. Artefakt-publisering til GitHub Releases
6. Kanari-utsetting og automatisk rollback
7. Nightly builds og pre-release kanaler
8. Dependabot-kompatibilitet og automatiske sikkerhetsoppdateringer

---

## Fase 12: CI/CD og release-automatisering (z293–z302)

Omgang 293–302 introduserte komplett CI/CD-pipeline og release-automatisering.

Siste dokumenterte store milepæl: Omgang 302.

Kjerne:

```text
git push → CI validate → tag → release pipeline → plattformbygg → GitHub Release → kanari
```

### z293 — GitHub Actions workflow-struktur og CI-jobbmodell
- `workflow`, `trigger`, `job`, `step`, `runner`, `matrix`
- `trigger_push/pull_request/schedule/workflow_dispatch/release`
- `job_needs`, `job_strategy`, `job_matrix`, `job_fail_fast`
- `step_uses`, `step_run`, `step_if`, `github_token`, `github_context`

### z294 — CI-pipeline for push og PR-validering
- `ci_job_check/test/lint/format_check/parity/audit`
- `setup_norscode`, `cache_step`, `cache_restore_keys`
- `junit_xml`, `upload_test_results`, `lint_annotations`
- `pr_status_check`, `required_checks`, `branch_protection`
- `commit_annotation`, `annotation_level`, `annotation_path`

### z295 — Release-pipeline utløst av versjonstagg
- `release_workflow`, `semver_tag`, `validate_tag`
- `version_match_check`, `changelog_entry_check`, `no_pre_release_deps`
- `build_matrix` per plattform, `artifact_upload/download`
- `github_release`, `release_body`, `release_assets`, `asset_upload`
- `extract_changelog_entry`, `release_notify`

### z296 — Kryssplattform-bygg og native binærproduksjon
- `universal_binary`, `lipo_merge`, `codesign`, `notarize`
- `musl_target`, `static_linux_binary`, `docker_build_image`
- `pe_binary`, `windows_signing`, `signtool`
- `wasm_opt`, `wasm_strip`, `wasm_size`
- `artifact_checksum`, `artifact_sign_file`

### z297 — Artefaktpakking og GitHub Releases-publisering
- `artifact_packager`, `tarball_builder`, `zip_builder`
- `package_layout`, `install_script`, `checksums_file`
- `release_publisher`, `gh_release_create`, `gh_asset_upload`
- `release_index`, `update_latest_pointer`, `latest_json`
- `cdn_publish`, `cdn_invalidate`

### z298 — Kanari-utsetting og automatisk rollback
- `canary_release`, `canary_percentage`, `canary_increment`
- `rollout_controller`, `rollout_state`, `health_check`
- `metric_collector`, `crash_rate`, `error_rate`, `metric_threshold`
- `rollout_gate`, `promote_canary`, `promote_full`
- `auto_rollback`, `rollback_to_version`, `nc_update_command`

### z299 — Nightly builds og pre-release kanaler
- `nightly_workflow`, `nightly_cron`, `nightly_version`, `nightly_build_id`
- `nightly_prune_policy`, `max_nightly_builds`, `prune_older_than`
- `pre_release`, `kind_alpha/beta/rc`, `pre_release_tag`
- `channel_nightly/beta/rc/stable`, `channel_manifest`, `switch_channel`
- `fuzz_run`, `fuzz_corpus`, `fuzz_crash`, `fuzz_minimize`

### z300 — Sikkerhetsscanning og sårbarhetsadministrasjon
- `sast`, `sast_finding`, `sarif_output`, `upload_sarif`
- `dependency_scan`, `dep_vulnerability`, `advisory_database`, `advisory_db_update`
- `audit_fix`, `auto_update_patch`
- `secret_scanner`, `secret_finding`, `allowed_patterns`
- `sbom`, `sbom_spdx`, `sbom_cyclonedx`, `sbom_attest`
- `dependabot_config`, `dependabot_schedule`

### z301 — CI-cache og pipeline-ytelsesoptimalisering
- `dep_cache_key`, `build_cache_ci`, `incremental_cache`, `tool_cache`
- `split_test_matrix`, `test_shard`, `shard_index`, `shard_count`
- `skip_unchanged`, `path_based_skip`, `affected_packages`, `change_detector`
- `reuse_artifacts`, `artifact_fingerprint`, `fingerprint_match`
- `timing_report`, `slowest_step`, `bottleneck_step`

### z302 — CI/CD-integrasjon, overvåking og observabilitet
- `build_badge`, `badge_ci/coverage/version`
- `coverage_tool`, `coverage_threshold`, `coverage_upload`, `bench_regression`
- `bench_comment_pr`, `continuous_benchmark`
- `pipeline_flakiness`, `flake_rate`, `quarantine_flaky`, `retry_flaky`
- `deployment_log`, `log_entry`, `log_actor`
- `environment_protection`, `required_reviewers`, `deployment_approval`

### Nåværende modell etter Omgang 302

```text
git push (branch/PR):
  trigger_push/pull_request
  -> ci_job_check + ci_job_test (sharded) + ci_job_lint + ci_job_format_check
     + ci_job_parity + ci_job_audit + sast + secret_scanner
  -> cache: dep_cache + build_cache + tool_cache
  -> path_based_skip (docs-only → hopp over bygg)
  -> required_checks -> PR merge gate

git tag v1.2.3:
  release_workflow -> validate_tag + changelog_entry_check
  -> build_matrix (macOS arm64/x64, Linux x64/arm64 musl, Windows x64, WASM)
  -> artifact_packager + checksums_file
  -> sbom_generate + sbom_attest
  -> gh_release_create + asset_upload
  -> registry publish
  -> canary_release (5% → 100% med health_check + metric_threshold)
  -> auto_rollback ved gate_fail

nightly (02:00 UTC):
  nightly_version (date+sha) -> full bygg + extended_test + fuzz_run
  -> channel_nightly oppdatert -> nightly_prune

sikkerhet (ukentlig):
  advisory_db_update -> dependency_scan -> auto_update_patch PR
  sast + secret_scanner på all kode
```

### Neste naturlige fase etter z302

Neste store omgang bør være **dokumentasjonsside og læringssenter** (z303+):

1. Statisk nettstedgenerator i Norscode
2. Språkreferanse auto-generert fra kildekode
3. Standardbibliotek-dokumentasjon
4. Interaktiv playground (WASM i nettleser)
5. Tutorial-system med steg-for-steg eksempler
6. Søkemotor for dokumentasjon
7. Versjonert dokumentasjon (per release)
8. Norskspråklig terminologiordliste

---

## Fase 13: Dokumentasjonsside og læringssenter (z303–z312)

Omgang 303–312 introduserte komplett dokumentasjonsside og interaktivt læringssenter.

Siste dokumenterte store milepæl: Omgang 312.

Kjerne:

```text
kilde + doc-kommentarer -> site_builder -> statisk HTML -> CDN -> norscode.dev
```

### z303 — Statisk nettstedgenerator og byggpipeline
- `site_builder`, `site_config`, `page`, `section`, `template_engine`
- `markdown_parser`, `frontmatter`, `asset_pipeline`, `asset_fingerprint`
- `build_pipeline`: `collect_content` → `render_pages` → `write_output`
- `build_sitemap`, `build_feed`, `build_search_index`
- `dev_server`, `dev_watch`, `dev_reload`

### z304 — Språkreferanse auto-generert fra kildekode
- `lang_ref_builder`, `keyword_entry`, `operator_ref`, `type_ref`
- `statement_ref`, `stmt_syntax`, `stmt_errors`
- `control_flow_ref`: hvis, ellers, mens, for, bryt, fortsett, returner, prøv/fang/kast
- `grammar_rule`, `grammar_ebnf`, `grammar_diagram` (SVG railroad diagrams)
- `example_snippet`, `snippet_runnable`, `cross_ref`

### z305 — Standardbibliotek-dokumentasjon
- `stdlib_doc_builder`, `doc_extractor`, `doc_comment`, `doc_summary`
- `doc_param_tag`, `doc_return_tag`, `doc_throws_tag`, `doc_example_tag`
- `func_doc`, `struct_doc`, `module_doc`, `field_doc`
- `stdlib_index`, `index_by_module`, `index_by_name`
- `module_nav`, `func_source_link`

### z306 — Interaktiv playground (WASM i nettleser)
- `playground`, `playground_embed`, `playground_standalone`
- `editor` (CodeMirror 6), `norscode_language_support`, `syntax_highlighting_wasm`
- `wasm_runner`, `compile_source`, `execute_wasm`, `sandbox`
- `sandbox_fuel_limit`, `sandbox_io_capture`
- `share_button`, `share_encode/decode`, `example_picker`

### z307 — Tutorial-system med steg-for-steg validering
- `tutorial`, `tutorial_step`, `step_validation`, `step_validation_rule`
- `rule_output_contains`, `rule_compiles`, `rule_type_checks`
- `progress_tracker`, `hint_system`, `hint_level`, `reveal_solution`
- `kom_i_gang`-spor: installasjon → første program → variabler → funksjoner → strukturer
- `learning_path`, `code_challenge`, `challenge_tests`

### z308 — Søkemotor for dokumentasjon
- `search_index`, `index_builder`, `tokenize_text`, `stem_token`, `stop_words`
- `bm25_score`, `title_boost`, `keyword_boost`, `rank_results`
- `search_result`, `result_snippet`, `result_highlights`
- `client_search`, `search_modal` (Ctrl+K), `keyboard_nav`
- `search_debounce`, `no_results_message`, `search_suggestions`

### z309 — Versjonert dokumentasjon
- `versioned_docs`, `doc_version`, `version_status`, `version_switcher`
- `version_build`, `version_deploy_path`, `version_manifest`
- `canonical_url`, `noindex_old_versions`, `archive_policy`
- `migration_guide`, `migration_change_entry`, `change_before/after`
- `deprecation_notice`, `notice_banner`, `removed_in`, `replacement`

### z310 — Norsk terminologiordliste og lokalisering
- `glossary`, `term`, `term_norsk`, `term_engelsk`, `term_definition`
- `norsk_nøkkelord`, `nøkkelord_engelsk_ekvivalent`
- Fullstendig nøkkelordtabell: funksjon/function, hvis/if, mens/while osv.
- `oversettelse_tabell` (maskinlesbar JSON for navngivningslinter)
- `lokalisering`, `locale_no/en`, `locale_plural`
- `style_guide`, `tech_norsk_norm`, `loanword_policy`

### z311 — Kokebok, how-to-guider og praktiske eksempler
- `cookbook`, `recipe`: problem + løsning + forklaring
- Web-oppskrifter: HTTP-server, JSON API, HTML-maler, middleware
- Fil-oppskrifter: lese/skrive filer, traversere mapper, parse CSV
- CLI-oppskrifter: argumentparsing, delkommandoer, fremdriftslinje
- Dataoppskrifter: sortering, gruppering, filter/map, akkumulering
- Feiloppskrifter: propagering, tilpassede feiltyper, retry, timeout

### z312 — Nettsteddeployering og bidragsarbeidsflyt
- `deploy_pipeline`, `deploy_preview`, `preview_pr_comment`
- `hosting_cloudflare_pages`, `set_redirects`, `set_headers`
- `cache_control_immutable/html`, `csp_header`, `hsts_header`
- `analytics` (cookie-fritt, respekterer DNT)
- `feedback_widget`, `page_rating`, `feedback_store`
- `broken_link_checker`, `sitemap_submit`

### Nåværende modell etter Omgang 312

```text
norscode.dev (statisk, CDN-levert):
  site_builder:
    lang_ref_builder    -> /referanse/ (alle nøkkelord, operatorer, typer, grammatikk)
    stdlib_doc_builder  -> /stdlib/    (alle moduler med doc-kommentarer)
    tutorial_builder    -> /lær/       (kom-i-gang-spor + interaktiv validering)
    cookbook_builder    -> /kokebok/   (oppskrifter + how-to-guider)
    glossary_builder    -> /ordliste/  (norsk terminologi + nøkkelordtabell)
    playground          -> /prøv/      (full editor + WASM-kompilering i nettleser)
    search_index        -> search.json (BM25, offline, Ctrl+K)
  versioned_docs: /docs/v1.2/, /docs/v1.1/, ... + version_switcher
  deploy_pipeline: push → bygg → CDN + preview per PR
  feedback_widget: per-side vurdering → feedback_report
  analytics: privacy-first, ingen cookies
```

### Neste naturlige fase etter z312

Neste store omgang bør være **native optimizer og ytelsespipeline** (z313+):

1. IR-optimeringer: constant folding, dead code elimination, inlining
2. Registerallokering (graph-coloring, x86_64 og AArch64)
3. Loop-optimeringer: LICM, loop unrolling, strength reduction
4. Profilstyrt optimering (PGO): instrumentering og bruksprofil
5. Benchmark-suite og ytelsesregresjonsdetektor
6. Peephole-optimeringer og instruksjonsvalg
7. Escape analysis og stack allocation
8. Link-time optimization (LTO)

---

## Fase 14: Native optimizer og ytelsespipeline (z313–z322)

Omgang 313–322 introduserte komplett optimeringspipeline fra IR til binær.

Siste dokumenterte store milepæl: Omgang 322.

Kjerne:

```text
typed_ir -> opt_pipeline -> regalloc -> peephole -> machine_code -> LTO -> binær
```

### z313 — IR-optimaliseringsrammeverk og pass manager
- `opt_pipeline`, `pass_manager`, `pass_kind` (function/module/loop)
- `analysis_cache`: `dominator_tree`, `loop_info`, `alias_analysis`, `call_graph`
- `pass_changed`, `analysis_invalidate`, `worklist`
- `opt_level`: none/basic/standard/aggressive
- `opt_stat`, `opt_report`

### z314 — Konstantfold og algebraisk forenkling
- `const_fold_pass`: fold_add/sub/mul/div/compare/cast
- `algebraic_simplify`: x+0→x, x*1→x, x*0→0, !!x→x
- `strength_reduce`: x*8→x<<3, x/4→x>>2, x%16→x&15
- `const_propagate`, `known_map`, `propagate_into_uses`
- `sparse_conditional_constant` (SCC): lattice-basert analyse

### z315 — Dead code elimination og utilgjengelig kode
- `dce_pass`: `liveness_pass`, `zero_uses`, `side_effect_free`
- `reachability_pass`: BFS fra entry → `prune_unreachable_blocks`
- `dead_branch_elim`: kjent betingelse → `fold_unconditional_branch`
- `dse_pass`: `subsequent_store` uten load → `remove_dead_store`
- `dead_alloc_elim`, `dfce_pass`, `global_dce`

### z316 — Funksjonsinlining og kallstedoptimering
- `inline_pass`, `inline_cost`, `inline_threshold`, `always_inline`
- `clone_callee`, `substitute_args`, `redirect_returns`, `merge_into_caller`
- `devirtualize`, `speculative_devirt`, `devirt_guard`
- `partial_inlining`: inline hot_path, ekstraher cold_path
- `ipa_const_propagate`, `ipa_dead_arg`, `remove_dead_arg`

### z317 — Loop-optimeringer: LICM, unrolling, vektorisering
- `licm_pass`: `loop_invariant_instr` → `hoist_instr` til preheader
- `loop_unroll`: `unroll_full` (kjent tripcnt) / `unroll_partial` + remainder
- `loop_unswitch`: `loop_invariant_condition` → klon løkke for hver gren
- `strength_reduce_loop`: multiplikasjon → akkumulering
- `loop_vectorize`: `dep_check` + `vector_cost_model` → SIMD + `scalar_epilogue`
- `loop_interchange`, `loop_fusion`, `eliminate_loop`

### z318 — Registerallokering: graph-coloring og spilling
- `interference_graph`, `graph_color`: simplify → coalesce → select → spill
- `conservative_coalesce` / `aggressive_coalesce`
- `spill_cost`, `spill_slot`, `spill_store/load`, `rematerialize`
- `linear_scan` (raskere, lavere opt-nivå)
- `register_hint`, `copy_propagate_reg`
- x86_64: rax/rbx/../r15, xmm0-15 | AArch64: x0-x30, v0-v31

### z319 — Profilstyrt optimering (PGO)
- `instrument_pass`: `edge_counter`, `block_counter`, `value_profile`
- `profile_data`, `profile_merge` (multiple runs)
- `block_frequency`, `branch_probability`, `hot_threshold`
- `pgo_inline` (aggressiv for varme kallsted), `pgo_inline_cold_suppress`
- `pgo_layout`: `hot_function_first`, `hot_blocks_together`, `section_cold`
- `pgo_devirt`: `value_histogram` → toppverdi → spekulativ direkte kall
- `sample_pgo`, `coverage_guided`

### z320 — Peephole-optimeringer og instruksjonsvalg
- `peephole_pass`: sliding window, `peephole_rule` (before → after)
- `eliminate_move`, `forward_store_value`, `compare_branch_fuse`, `fma_instr`
- `zero_idiom`: mov 0 → xor-self; `fall_through_opt`
- `instruction_select`: `tile_ir` + `cost_tile` → `emit_machine_instr`
- x86_64: `use_lea`, `use_cmov`, `use_test`, `use_movzx`
- AArch64: `use_madd/msub`, `use_csel`, `use_ldp_stp`, `use_ubfx`

### z321 — Escape analysis og stack allocation
- `escape_analysis`: points_to_graph → `escaped_value` / `non_escaped_value`
- `escape_reason`: returned, stored_global, captured_closure, address_taken
- `stack_alloc`: `non_escaped` → `replace_heap_alloc` → `stack_slot`
- `sroa_pass`: split aggregat → `scalar_field` per felt
- `alloc_elim`: aldri escaped + aldri lest → `eliminate_allocation`
- `promote_capture`: lukket variabel som ikke endres → by-value
- `capture_on_stack`: lukking som ikke escaper → miljø på stakken

### z322 — Link-time optimization og benchmark-suite
- `lto_full`: merge all IR → full optimizer pipeline → binær
- `lto_thin`: `thin_summary_file` → `thin_import_list` → parallell codegen + `thin_cache`
- `lto_internalize`, `lto_dce_cross_module`, `lto_inline_cross_module`
- `bench_suite`: fibonacci, sort, json_parse, string_ops, list/map_ops, compile_self
- `bench_runner`: warmup + iterations → mean/stddev/min/max/throughput
- `bench_compare`: `regression_threshold` → CI-feil + `report_pr_comment`
- `bench_history`, `bench_trend`

### Nåværende modell etter Omgang 322

```text
typed_ir
-> opt_pipeline (pass_manager, opt_level):
   const_fold + algebraic_simplify + strength_reduce
   const_propagate (SCC)
   dce (liveness + zero_uses)
   unreachable block prune + dead_branch_elim
   dse + dead_alloc_elim + global_dce
   inline_pass (cost-based + always_inline + ipa_const_prop)
   devirtualize + speculative_devirt
   licm + loop_unroll + loop_unswitch
   strength_reduce_loop + loop_vectorize
   escape_analysis -> stack_alloc + sroa + alloc_elim
   [PGO feedback: pgo_inline + pgo_layout + pgo_devirt]
-> regalloc (graph_color or linear_scan)
   spill + rematerialize + coalesce
-> peephole + instruction_select
   fma_fuse + zero_idiom + compare_branch_fuse
   x86_64/AArch64 idioms
-> LTO (thin eller full)
   cross-module inline + dce + const_prop + internalize
-> binær

Benchmark: bench_suite i CI
   -> bench_compare(current, baseline) -> regression_threshold
   -> bench_history + bench_trend
```

### Neste naturlige fase etter z322

Neste store omgang bør være **distribuert kjøretid og cloud-native** (z323+):

1. Distribuert meldingssystem og aktørmodell
2. Horisontal skalering og lastbalansering
3. Distribuert tilstand og konsensus
4. Observabilitet: traces, metrics og logs (OpenTelemetry)
5. Service mesh og tjenesteoppdag
6. Feil-toleranse og circuit breaker
7. Distribuert cache og datalagring
8. Cloud-native deployment (Kubernetes, containere)

---

## Fase 15: Distribuert kjøretid og cloud-native (z323–z332)

Omgang 323–332 introduserte komplett distribuert kjøretid og cloud-native infrastruktur.

Siste dokumenterte store milepæl: Omgang 332.

Kjerne:

```text
aktørmodell + konsensus + observabilitet + service mesh + feil-toleranse
+ Kubernetes + API-gateway + cloud-orkestrasjon
```

### z323 — Aktørmodell og distribuert meldingsutveksling
- `actor`, `actor_ref`, `actor_mailbox`, `mailbox_overflow`
- `tell` (asynk), `ask` (synk med timeout), `fire_and_forget`
- `receive_loop`, `stash`, `supervision`, `supervisor_strategy`
- `strategy_restart/stop/escalate/resume`, `failure_budget`
- `location_transparent`, `remote_ref`, `serialize_message`

### z324 — Horisontal skalering og lastbalansering
- `cluster`, `gossip_protocol`, `phi_accrual`, `failure_detector`
- `load_balancer`: round_robin, least_conn, consistent_hash, power_of_two
- `health_probe`, `unhealthy_threshold`, `upstream_unhealthy`
- `shard`, `consistent_hash_ring`, `virtual_node`, `rebalance_shards`
- `autoscaler`, `scale_metric`, `scale_cooldown`, `min/max_replicas`

### z325 — Distribuert tilstand og konsensus (Raft)
- `raft_node`: leader/follower/candidate, `raft_term`, `raft_log`
- `leader_election`, `request_vote`, `quorum`, `majority`
- `log_replication`, `replicate_entry`, `commit_if_majority`
- `state_machine`, `sm_snapshot`, `sm_restore`
- `distributed_kv`, `kv_watch`, `kv_lease`
- `distributed_lock`, `lock_fencing_token`
- `crdt`, `crdt_merge`, `eventual_consistency`

### z326 — Observabilitet (OpenTelemetry)
- `tracer`, `span`, `trace_id`, `span_kind`, `span_attr`, `span_event`
- `w3c_trace_context`, `traceparent`, `extract_context`, `inject_context`
- `counter`, `histogram`, `gauge`, `exemplar`
- `log_record`, `log_level`, `log_span_context`
- `otlp_exporter` (gRPC/HTTP), `prometheus_exporter`, `export_batch`

### z327 — Service mesh og tjenesteoppdag
- `sidecar_proxy`, `data_plane`, `control_plane`
- `service_registry`, `dns_discovery`, `watch_services`
- `mTLS`, `spiffe_id`, `cert_rotation`
- `traffic_split`, `canary_route`, `mirror_traffic`
- `retry_policy`, `timeout_policy`, `outlier_detection`
- `ejection_percent`, `base_ejection_time`

### z328 — Feil-toleranse og circuit breaker
- `circuit_breaker`: closed/open/half_open, `cb_trip`, `cb_reset`
- `bulkhead`: semaphore/thread_pool, `bulkhead_rejected`
- `token_bucket`, `leaky_bucket`
- `retry_backoff_exp`, `retry_jitter`, `deadline`
- `fallback_fn`, `fallback_cache`, `stale_if_error`
- `hedge`, `shed_load`, `priority_shed`
- `chaos_engineering`, `fault_inject`, `inject_latency/error`

### z329 — Distribuert cache og datalagring
- `dist_cache`: single/replicated/partitioned/hybrid
- `eviction_policy`: LRU, LFU, ARC, TTL
- `write_through/behind/around`, `read_through`, `stale_while_revalidate`
- `invalidate_tag`, `cache_flush`
- `consistency_level`: strong/bounded_staleness/session/eventual
- `replication_factor`, `primary/secondary_replica`
- `compaction`, `tombstone`, `write_amplification`

### z330 — Container-runtime og Kubernetes
- `multi_stage_build`, `distroless_base`, `image_digest`
- `oci_spec`, `cgroup_v2`, `cpu/memory_limit`
- `pod`, `liveness/readiness/startup_probe`
- `deployment`, `rolling_update`, `max_unavailable/surge`
- `hpa`, `hpa_metric`, `hpa_scale_up/down`
- `helm_chart`, `helm_upgrade`, `helm_rollback`
- `kustomize_overlay`, `kustomize_patch`

### z331 — API-gateway og cloud-native ingress
- `api_gateway`, `gateway_route`, `request_pipeline`
- `jwt_auth`, `oauth2_introspect`, `api_key_auth`
- `rate_limit_plugin`: per_ip/user/api_key, `rl_retry_after`
- `transform_plugin`: rewrite_path, strip_prefix, add_cors
- `response_cache`, `stale_while_revalidate`, `bypass_cache`
- `cert_manager`, `lets_encrypt`, `acme_challenge`
- `websocket_support`

### z332 — Distribuert runtime-integrasjon og cloud-orkestrasjon
- `infrastructure_as_code`: `iac_plan`, `iac_apply`, `iac_drift_detect`
- `managed_services`: db, cache, queue, object_storage, secret_store
- `event_bus`: `at_least_once`, `exactly_once`, `dead_letter_queue`
- `workflow_engine`: `durable_execution`, `workflow_replay`, `workflow_history`
- `distributed_scheduler`, `job_idempotency_key`
- `multi_region`, `geo_routing`, `failover_region`, `data_residency`
- `gitops`, `reconcile_loop`, `desired_state`, `drift_detected`

### Nåværende modell etter Omgang 332

```text
Distribuert Norscode-applikasjon:

Aktørlag:        aktørmodell + mailbox + supervision + location_transparent
Klusterlag:      gossip + phi_accrual + consistent_hash + autoscaler
Konsensus:       Raft (leader election + log replication) + distributed_kv
Observabilitet:  OTel traces + metrics + logs + OTLP export + Prometheus
Service mesh:    sidecar + mTLS + spiffe_id + traffic_split + retry/timeout
Feil-toleranse:  circuit_breaker + bulkhead + token_bucket + hedge + chaos
Cache/lagring:   consistent_hash_ring + LRU + write_through + strong/eventual
Kubernetes:      pod + rolling_update + HPA + Helm + Kustomize
API-gateway:     JWT/OAuth2 + rate_limit + transform + response_cache + CORS
Cloud:           IaC + managed_services + event_bus + workflow_engine + GitOps
```

### Neste naturlige fase etter z332

Neste store omgang bør være **AI og maskinlæring-integrasjon** (z333+):

1. Innebygd ML-inferens-kjøretid
2. Vektordatabase og semantisk søk
3. LLM-integrasjon og prompt-pipeline
4. RAG (Retrieval-Augmented Generation)
5. Strømmende LLM-svar og token-håndtering
6. Embeddings og semantisk likhetsberegning
7. AI-agent-rammeverk med verktøy og minne
8. Evaluering og observabilitet for AI-systemer

---

## Fase 16: AI og maskinlæring-integrasjon (z333–z342)

Omgang 333–342 introduserte komplett AI og ML-integrasjon i Norscode.

Siste dokumenterte store milepæl: Omgang 342.

Kjerne:

```text
inferens-runtime + vektor-DB + LLM-klient + RAG + agenter + evaluering
```

### z333 — ML-inferens-kjøretid og tensoroperasjoner
- `inference_runtime`: CPU/GPU/NPU/WASM-backends
- `model_format`: ONNX, safetensors, GGUF
- `tensor`, `tensor_shape`, `dtype_f32/f16/bf16/i8`
- `op_matmul`, `op_gelu`, `op_softmax`, `op_layer_norm`, `op_rms_norm`
- `quantization`: int8/int4, `quant_scale`, `quant_zero_point`, `dequantize`
- `session`, `input_binding`, `output_binding`

### z334 — Vektordatabase og semantisk søk
- `vector_db`, `vdb_collection`, `distance_cosine/euclidean/dot_product`
- `index_hnsw`: `hnsw_m`, `hnsw_ef_construction/search`
- `upsert`, `search`, `search_filter` (must/should/must_not)
- `sparse_vector`, `bm25_vector`
- `hybrid_search`, `hybrid_rrf`, `rrf_k`, `fusion_score`
- `snapshot_create`, `collection_alias`

### z335 — LLM-klient og prompt-pipeline
- `llm_client`: Anthropic, OpenAI, Mistral, Ollama, lokal
- `message_role`: system/user/assistant/tool
- `content_tool_use`, `content_tool_result`, `content_image`
- `prompt_template`, `template_render`, `prompt_registry`, `prompt_version`
- `completion_request`: temp, top_p, stop_sequences, tools
- `prompt_cache`, `cache_control_persistent`, `cache_read_tokens`
- `count_tokens`, `trim_history`, `history_token_budget`

### z336 — Strømmende LLM-svar og token-håndtering
- `stream_event`: message_start/content_block_delta/message_stop
- `delta_text`, `delta_input_json`, `delta_thinking`
- `sse_parser`, `parse_sse_line`, `sse_dispatch`
- `text_accumulator`, `partial_json`, `json_stream_parser`
- `backpressure`, `pause_stream`, `resume_stream`
- `time_to_first_token`, `tokens_per_second`
- `thinking_block`, `thinking_signature`, `redacted_thinking`

### z337 — RAG (Retrieval-Augmented Generation)
- `chunker`: fixed_size/sentence/paragraph/recursive/semantic
- `chunk_overlap`, `embedder.embed_batch`, `embed_cache`
- `indexer` → `vector_db`, `retriever.retrieve(top_k, filter)`
- `reranker`, `cross_encoder_rerank`, `rerank_score`
- `context_builder`: dedup + sort + cite_sources
- `query_rewrite`, `multi_query`, `hypothetical_document` (HyDE)
- `faithfulness`, `answer_relevance`, `context_precision/recall`

### z338 — Embeddings og semantisk likhet
- `embedding_model`: tekst/kode/multimodal
- `cosine_similarity`, `euclidean_distance`, `similarity_threshold`
- `semantic_cluster`: kmeans/hierarchical/dbscan
- `dimensionality_reduction`: PCA, UMAP, t-SNE → 2D/3D
- `semantic_dedup`, `dedup_threshold`, `canonical_doc`
- `cross_lingual`, `multilingual_model`, `align_embeddings`
- `fine_tune_embeddings`: contrastive learning, triplet_loss

### z339 — AI-agent-rammeverk med verktøy og minne
- `agent`, `agent_run`, `step`: thought → action → observation
- `tool_registry`, `tool_handler`, `builtin_tools`
- `tool_web_search/read_file/write_file/run_code/http_request`
- `working_memory`, `wm_token_budget`
- `episodic_memory`: recall_episodes, forget, episode_importance
- `semantic_memory_agent`: remember_fact, recall_fact
- `memory_consolidation`, `summarize_memory`
- `multi_agent`: orchestrator, sub_agent, agent_handoff, shared_memory

### z340 — AI-observabilitet og evaluering
- `span_llm_call/tool_call/embedding/retrieval/rerank`
- `attr_cost_usd`, `attr_input/output_tokens`, `prompt_trace`
- `eval_suite`, `evaluator`: exact_match/regex/llm_judge
- `judge_score`, `judge_reasoning`
- `metric_faithfulness/hallucination/toxicity/bias`
- `regression_eval`, `eval_regression_threshold`
- `ab_test_ai`, `statistical_significance`
- `guardrail`: block_toxic/PII, `prompt_injection_detect`, `jailbreak_detect`

### z341 — Modell-finjustering og modellhåndtering
- `fine_tune`, `ft_job`: LoRA (rank, alpha, dropout, target_modules)
- `training_data`: JSONL, chat-format, validering
- `ft_hyperparams`: lr, scheduler, batch_size, num_epochs
- `ft_checkpoint`, `early_stopping`, `patience`
- `model_registry`: versjon, artifact, tags, lineage
- `promote_to_prod`, `serve_canary_ml`, `serve_shadow`, `serve_rollback_ml`
- `continuous_training`: data_drift, concept_drift, retrain_pipeline

### z342 — AI-pipeline-integrasjon og applikasjonsmønstre
- `ai_pipeline`: preprocess → embed → retrieve → rerank → augment → generate → postprocess → evaluate
- `cost_tracker`, `budget_daily_usd`, `budget_alert_threshold`
- `batch_processor`, `batch_parallel`
- `structured_output`: json_mode / tool_use_extraction + `retry_on_invalid`
- `classifier`: zero_shot/few_shot, `classification_threshold`
- `summarizer`: bullet/paragraph/executive, extractive/abstractive
- `translation`, `chat_interface`, `inject_context_chat`
- `norscode_ai_stdlib`: std_ai_llm/embed/rag/agent/eval
- `nc ai run`, `nc ai eval`, `nc ai search`

### Nåværende modell etter Omgang 342

```text
Norscode AI-applikasjon:

Inferens:     ONNX/GGUF session → tensor_ops (matmul, softmax, layer_norm)
              quantization (int8/int4, LoRA)
Vektor-DB:    HNSW-indeks + hybrid_search (dense + BM25) + RRF-fusjon
LLM-klient:   multi-provider + prompt_cache + streaming SSE + thinking
RAG:          chunk → embed → index → retrieve → rerank → context → generate
Agenter:      reasoning loop + tool_registry + working/episodic/semantic memory
              multi_agent orchestration + agent_handoff
Evaluering:   llm_judge + faithfulness/hallucination + regression_eval
              guardrails (toxic/PII/injection/jailbreak)
Fintuning:    LoRA fine-tune → model_registry → canary serve → drift retrain
Pipeline:     ai_pipeline (compose stages) + batch_processor + structured_output
stdlib:       std.ai.llm / std.ai.embed / std.ai.rag / std.ai.agent / std.ai.eval
CLI:          nc ai run / nc ai eval / nc ai search / nc ai embed
```

### Neste naturlige fase etter z342

Neste store omgang bør være **sikkerhet og kryptografi** (z343+):

1. Kryptografisk primitiver (AES, ChaCha20, RSA, ECC)
2. Nøkkelhåndtering og nøkkellagre (KMS, HSM)
3. TLS/mTLS-implementasjon
4. Autentisering og autorisasjon (OIDC, RBAC)
5. Sikkerhets-tokens: JWT, PASETO, Macaroons
6. Hashing, HMAC og digitale signaturer
7. Hemmelige-håndtering og rotation
8. Sikker lagring og kryptert backup

---

## Fase 17: Sikkerhet og kryptografi (z343–z352)

Omgang 343–352 introduserte komplett kryptografi og sikkerhets-infrastruktur.

Siste dokumenterte store milepæl: Omgang 352.

Kjerne:

```text
symmetrisk AEAD + asymmetrisk krypto + TLS + autentisering + autorisasjon
+ tokens + KMS + hemmeligheter + sikker lagring + security stdlib
```

### z343 — Symmetrisk kryptografi (AES-GCM, ChaCha20-Poly1305)
- `aes_gcm`: nonce, tag, aad, `gcm_encrypt/decrypt`, `gcm_verify_tag`
- `chacha20_poly1305`, `xchacha20` (utvidet nonce)
- `aead`: unified interface over GCM og ChaCha20-Poly1305
- `generate_key`, `zeroize_key`, `random_nonce`, `nonce_reuse_detect`
- `pkcs7_pad/unpad`, `padding_error`

### z344 — Asymmetrisk kryptografi (RSA, ECC, Ed25519, X25519)
- `rsa_oaep` (kryptering), `rsa_pss` (signering)
- `ec_curve`: P-256/384/521, secp256k1
- `ecdsa_sign/verify`, `der_encode_sig`
- `ed25519_sign/verify` (deterministisk)
- `x25519_dh` (Diffie-Hellman, forward secrecy)
- `hybrid_encrypt`: `kem_encapsulate/decapsulate` + AEAD

### z345 — Hashing, HMAC og passordhashing
- SHA-256/384/512, SHA-3, BLAKE3 (keyed + context)
- `hmac_sha256/512`, `hmac_verify` (constant-time)
- `hkdf`: `extract` + `expand`, `hkdf_info` (domain separation)
- `pbkdf2` (600k iterasjoner min), `scrypt`
- `argon2id` (anbefalt): memory/iterations/parallelism, PHC-streng
- `password_needs_rehash`, `upgrade_hash`
- `merkle_tree`, `merkle_proof`, `merkle_verify_proof`

### z346 — TLS 1.3 og sertifikathåndtering
- `tls_1_3`, `ecdhe_key_exchange`, `session_ticket`, `zero_rtt`, `anti_replay`
- `cipher_suite`: AES-128-GCM-SHA256, AES-256-GCM-SHA384, ChaCha20-Poly1305
- `certificate`, `cert_san`, `cert_extensions`, `chain_verify`, `path_building`
- `crl_check`, `ocsp_staple`
- `mtls`: `verify_client_cert`, `peer_identity`, `spiffe_uri`
- `acme`, `acme_challenge_http/dns`, `auto_renew`, `cert_manager_tls`

### z347 — Autentisering: OIDC, OAuth2 og sesjoner
- `authorization_code_flow`, `pkce` (code_verifier + challenge)
- `id_token_verify`: `jwks`, `kid_header`, `audience_check`, `nonce_oidc`
- `token_introspect`, `token_active`
- `session`: `session_id`, `session_rotate_id`, `sliding/absolute_expiry`
- `csrf_token`, `double_submit_cookie`, `same_site_strict`
- `totp_verify` (RFC 6238), `webauthn_authenticate`, `recovery_code`

### z348 — Autorisasjon: RBAC, ABAC og policy engine
- `rbac`: `role`, `role_hierarchy`, `inherited_permissions`, `rbac_check`
- `abac`: `policy`, `effect_allow/deny`, `policy_conditions`, `condition_operator`
- `policy_engine`: `engine_evaluate`, `policy_decision`
- `opa`: `opa_input`, `opa_query`, `opa_bundle` (Rego)
- `row_level_security`, `rls_filter`, `data_mask`, `field_mask`
- `audit_decision`, `audit_record`

### z349 — Sikkerhets-tokens: JWT, PASETO og API-nøkler
- `jwt_sign/verify`, `alg_eddsa` (anbefalt), `alg_es256`
- `jwt_claims`: iss/sub/aud/exp/nbf/iat/jti
- `jwks_endpoint`, `kid_header`, `jwks_cache` (nøkkelrotasjon)
- `paseto_v4_local` (ChaCha20) + `paseto_v4_public` (Ed25519)
- `paseto_implicit_assertion` (kontekst-binding)
- `api_key_generate/verify/revoke`, `api_key_prefix`, `api_key_hash`
- `token_blocklist`, `jti_check`
- `dpop_proof`, `sender_constrained`

### z350 — Nøkkelhåndtering: KMS og HSM
- `kms_provider`: AWS KMS, GCP KMS, Azure Key Vault, Vault, lokal
- `envelope_encryption`: `kms_generate_data_key` → AEAD krypterer data
- `kms_sign` (nøkkel aldri forlater KMS)
- `hsm`, `pkcs11`, `pkcs11_key_handle`
- `key_hierarchy`: root_key → KEK → DEK
- `key_rotation`: `rotation_schedule`, `re_encrypt_data`
- `per_tenant_key`, `context_binding`
- `shamir_secret_sharing`: shards + threshold → `reconstruct_key`

### z351 — Hemmelighets-håndtering og rotasjon
- `vault`: `vault_auth_method` (K8s/AppRole/AWS), `vault_lease`, `lease_renew`
- `dynamic_secret`: `database_secret` → unike credentials per instans
- `secret_rotation`: `dual_active_secrets`, `rotation_step`
- `k8s_secret_sync`, `external_secrets_operator`, `inject_env`
- `secret_scan_sm`: `trufflehog`, `gitleaks`, `allowlist_sm`
- `just_in_time_access`, `temporary_credential`, `access_approval`

### z352 — Sikker lagring, herding og security stdlib
- `encrypt_file`, `file_header`, `secure_delete`, `overwrite_passes`
- `transparent_encryption`, `encrypt_page/decrypt_page`
- `secure_alloc`, `guard_page`, `canary_value`, `aslr`, `pie_binary`, `nx_bit`
- `validate_input`: path_traversal, command_inject, sql_inject
- `html_escape`, `url_encode`, `json_escape`
- `security_headers`: CSP, HSTS, X-Frame-Options, Permissions-Policy
- `security_stdlib`: `std.crypto` / `std.auth` / `std.authz` / `std.secrets` / `std.tls`
- `nc sec audit/scan/keygen/encrypt/decrypt/hash`

### Nåværende modell etter Omgang 352

```text
Norscode sikkerhets-stack:

Symmetrisk:    AES-256-GCM + ChaCha20-Poly1305 via AEAD-abstraksjon
Asymmetrisk:   Ed25519 (sign) + X25519 (DH) + hybrid KEM
Hashing:       BLAKE3 + SHA-3 + Argon2id (passord) + HKDF (KDF)
TLS:           TLS 1.3 + ECDHE + OCSP-staple + mTLS + ACME auto-renew
Autentisering: OIDC/OAuth2 + PKCE + JWT/PASETO + TOTP + WebAuthn
Autorisasjon:  RBAC + ABAC + OPA (Rego) + RLS + audit log
Tokens:        JWT (EdDSA) + PASETO v4 + API-nøkler + DPoP
KMS/HSM:       Envelope-kryptering + nøkkelhierarki + Shamir sharding
Hemmeligheter: Vault + dynamiske credentials + rotasjon + K8s sync
Lagring:       Krypterte filer + DB-sider + sikker sletting
Herding:       ASLR + guard pages + canary + NX-bit + input-validering
stdlib:        std.crypto / std.auth / std.authz / std.secrets / std.tls
CLI:           nc sec audit/scan/keygen/encrypt/decrypt/hash
```

### Neste naturlige fase etter z352

Neste store omgang bør være **databehandling og strøm-prosessering** (z353+):

1. Strøm-prosessering og event sourcing
2. Batch-databehandling og ETL-pipeline
3. SQL-query-motor og relasjonell algebra
4. Reaktiv dataflyt og back-pressure
5. Tidsseriedata og vindusoperasjoner
6. Graf-prosessering og traversal
7. Dataformat-serde (Parquet, Arrow, Avro)
8. Distribuert join og aggregering

---

## Fase 18: Databehandling og strøm-prosessering (z353–z362)

Omgang 353–362 introduserte komplett databehandlings-infrastruktur.

Siste dokumenterte store milepæl: Omgang 362.

Kjerne:

```text
event_log + stream_pipeline + ETL + SQL + reaktiv + tidsserie
+ graf + Parquet/Arrow/Avro + distribuert join + dataframe stdlib
```

### z353 — Strøm-prosessering og event sourcing
- `stream_processor`, `event_log`, `append_event`, `read_from_offset`
- `consumer_group`, `commit_offset`, `lag`
- `watermark`, `event_time`, `allowed_lateness`, `drop_late`
- `operator_map/filter/flatmap/reduce/scan/merge/zip`
- `event_sourcing`: `apply_event`, `rebuild_state`, `snapshot_agg`

### z354 — Batch-databehandling og ETL-pipeline
- `etl_pipeline`: extract → transform → load
- `read_page`, `page_token`, `write_mode`: append/overwrite/upsert
- `row_transform`: map, filter, cast, enrich, drop/rename columns
- `aggregate_transform`: group_by + agg_sum/count/avg/min/max/collect
- `join_transform`: inner/left_outer/right_outer/full_outer/cross
- `data_quality`: check_not_null/unique/range/pattern/referential → quarantine_sink

### z355 — SQL-query-motor og relasjonell algebra
- `sql_parse` → `sql_ast` → `logical_plan` → `physical_plan`
- RA-operatorer: scan, filter, project, hash_join, merge_join, aggregate, sort
- `query_planner`: push_filter, eliminate_projection, join_reorder, use_index
- `btree_index`, `hash_index`, `index_scan`, `index_range_scan`
- `statistics`, `cost_model`, `estimated_rows`, `batch_iterator`

### z356 — Reaktiv dataflyt og backpressure
- `publisher/subscriber/subscription`, `request_n`, `on_next/complete/error`
- `overflow_strategy`: buffer/drop/error/latest
- `observable`: create/just/from_list/interval/timer
- `op_rx_switchmap/debounce/throttle/sample/combine_latest`
- `publish_subject/behavior_subject/replay_subject`
- `schedule_on/observe_on`, `retry_when`, `on_error_resume`

### z357 — Tidsseriedata og vindusoperasjoner
- `ts_store`: `ts_write/read/range_query/compaction`
- `tumbling_window`, `sliding_window`, `session_window`
- `wf_rate/increase/delta/percentile/ewma`
- `downsample`: mean/max/min/sum/last + `interp_linear/step`
- `anomaly_detect`: z_score + iqr_bounds + moving_avg
- `rollup_rule`, `stale_marker`, promql/flux/sql_timeseries

### z358 — Graf-prosessering og traversal
- `graph`: directed/weighted, adjacency_list, CSR
- `bfs`, `dfs`: pre/post-order, `topological_sort`
- `dijkstra`, `bellman_ford`, `a_star`
- `connected_components`, `scc` (Tarjan), `bridge`, `articulation_point`
- `pagerank` (alpha + iterativ konvergens)
- `louvain`, `label_propagation`, `modularity`
- `graph_pattern`, Cypher/Gremlin

### z359 — Dataformat-serde (Parquet, Arrow, Avro)
- Parquet: row_group, column_chunk, encoding_rle/delta/dict, compress_snappy/zstd
- `parquet_statistics`, `bloom_filter`, `predicate_pushdown`, `column_projection`
- Arrow: `array_buffer`, `validity_bitmap`, `record_batch`, IPC, `flight_protocol`
- `arrow_compute`: vektoriserte kernel-funksjoner
- Avro: `avro_schema_registry`, `schema_id`, `schema_evolution`, avro_union

### z360 — Distribuert join og aggregering
- `shuffle`: hash_partition/range_partition → `shuffle_write/read`
- `broadcast_join` (< broadcast_threshold), `strategy_sort_merge_dist`
- `partial_aggregate` → `final_aggregate`, `combine_fn`
- `hyperloglog`: `hll_merge`, `hll_estimate` (distinct count)
- `skew_detect`, `salt_key`, `salted_join`
- `all_reduce/all_gather/reduce_scatter`
- `query_fragment`, `coordinator`, `stream_exchange`

### z361 — Pipeline-orkestrering og datalinagg
- `dag`: `dag_validate`, `cycle_detect`, `topological_order`
- `task_dp`: extract/transform/load/sql/python/norscode
- `run`: queued/running/success/failed + `task_instance`, `xcom_push/pull`
- `sensor`: file/sql/http/external_task
- `lineage_graph`: `edge_input/output`, `impact_analysis`, `root_cause_analysis`
- `data_catalog`: `catalog_dataset`, `dataset_quality_score`, `catalog_search`

### z362 — Databehandlings-stdlib og integrasjon
- `dataframe`: `df_from_csv/parquet/json`, `df_select/filter/groupby/agg/join`
- `df_pivot/unpivot`, `df_fillna/dropna`, `df_dedup`, `df_cast`
- `lazy_frame`: `lf_scan/filter/groupby/agg/collect/explain`
- `nc data run/schema/validate/convert/sample/stats/diff/merge`
- `pipeline_builder`: `pb_source/transform/sink/branch/merge/build/run/dry_run`
- `data_test`: `test_no_nulls/freshness/duplicate_free/column_range`

### Nåværende modell etter Omgang 362

```text
Norscode databehandlings-stack:

Strøm:        event_log + watermark + consumer_group + operator_scan
ETL:          extract(page) → transform(map/filter/cast/join) → load(upsert)
SQL:          parse → RA-plan → optimize(push_filter/index/join_order) → batch execute
Reaktiv:      Observable + backpressure + debounce/switchmap + subject
Tidsserie:    ts_chunk + tumbling/session_window + ewma + anomaly + rollup
Graf:         CSR + dijkstra + SCC(Tarjan) + PageRank + Louvain
Formater:     Parquet(bloom/pushdown) + Arrow(IPC/Flight) + Avro(schema registry)
Dist join:    shuffle(hash) + broadcast + HLL(distinct) + skew-salt + fragment
Orkestrering: DAG + sensor + XCom + lineage_graph + data_catalog
stdlib:       dataframe + lazy_frame + pipeline_builder + data_test
CLI:          nc data run/schema/validate/convert/stats/diff
```

### Neste naturlige fase etter z362

Neste store omgang bør være **nettverksprotokoll og socket-runtime** (z363+):

1. TCP/UDP socket-primitiver og asynkron I/O
2. HTTP/1.1 og HTTP/2 protokollimplementasjon
3. WebSocket og server-sent events
4. DNS-oppslag og navneoppløsning
5. gRPC og protobuf-koding
6. Proxy og tunneling (SOCKS5, HTTP CONNECT)
7. Nettverks-middlerware og request-pipeline
8. Nettverkssimulering og testing

---

## Fase 19: Nettverksprotokoll + Native UI (z363–z377)

Omgang 363–377 fullførte nettverksfasen og startet native UI-runtime.

Siste dokumenterte store milepæl: Omgang 377.

### Nettverksfasen (z363–z371)

```text
TCP/UDP → HTTP/1.1+2 → WebSocket/SSE → DNS → gRPC → Proxy → Middleware → Test → Stdlib
```

- **z363** TCP/UDP async I/O: socket, epoll/kqueue/io_uring, non-blocking, socket_pool
- **z364** HTTP/1.1+2: parser/writer, chunked, HTTP/2 framing, HPACK, flow control
- **z365** WebSocket/SSE: handshake, framing, masking, rooms, last-event-id
- **z366** DNS: query, wire-format, cache (negativt), SRV-oppdagelse, mDNS
- **z367** gRPC/protobuf: 4 streaming-mønstre, varints, interceptors, health, reflection
- **z368** Proxy/tunneling: HTTP CONNECT, SOCKS5/4a, reverse proxy, sticky session, SSL-terminering
- **z369** Nettverks-middleware: middleware_chain, router, logging, auth, rate_limit, CORS, compress
- **z370** Nettverkstesting: http_test_server, mock_handler, sim_network, fault_injection, packet_capture
- **z371** Nettverks-stdlib: tcp_listener, http_client, url_parse, CIDR, `nc net fetch/serve/dns/cert`

### Native UI-fasen (z372–z377)

```text
Widget-tre + Layout + Tegning + Input + Tema + Reaktiv UI stdlib
```

- **z372** UI-runtime: app_event_loop, window, widget-tre, on_click/hover/submit
- **z373** Layout: flex, grid, stack, text_measure, hit_test, geometry
- **z374** Tegning: canvas, paint, path, transform, clip, text_render, skygge (Metal/Vulkan/wgpu)
- **z375** Input/animasjon: keyboard, mouse, touch, gesture, spring-easing, transition, dark_mode
- **z376** Tema: design_token, color_scale, semantic_color, spacing, typography, radius, elevation
- **z377** UI-stdlib: view, use_state/effect/ref, signal, virtual_list, form, a11y, `nc ui preview`

### Nåværende modell etter Omgang 377

```text
Norscode nettverks-stack:
  TCP/UDP + epoll + HTTP/2 + HPACK + WS + SSE + DNS + gRPC + SOCKS5
  + reverse proxy + middleware chain + test server + net stdlib

Norscode native UI-stack:
  app_event_loop + window + widget_tree
  + flex/grid layout + canvas (Metal/Vulkan/wgpu)
  + input + gesture + spring-animation + dark_mode
  + design_token + color_scale + semantic_color
  + view + signal + virtual_list + form + a11y
  + nc ui preview/storybook/test
```

### Neste naturlige fase etter z377

Neste store omgang bør være **terminal og TUI runtime** (z378+):

1. Terminal I/O og ANSI escape-koder
2. Raw mode og mus-støtte i terminal
3. TUI widget-system (bokser, tabeller, lister)
4. Layout-motor for terminal (columns, rows, flex)
5. Farger, stil og temaer i terminal
6. Asynkron terminal-event-loop
7. TUI-animasjon og progressbar
8. `nc tui` — interaktivt CLI-rammeverk

---

## Fase 20: Terminal og TUI runtime (z378–z387)

Omgang 378–387 introduserte komplett TUI-rammeverk for Norscode.

Siste dokumenterte store milepæl: Omgang 387.

Kjerne:

```text
ANSI escapes + raw_mode + cell_buffer + layout + farger
+ event_loop + animasjon + input + app_framework + stdlib
```

- **z378** Terminal I/O: ANSI CSI/OSC, cursor_goto, alternate_screen, hyperlinks, SIGWINCH
- **z379** Raw mode: termios, escape-sekvens-timeout, mouse_sgr, bracketed_paste, kitty_keyboard
- **z380** TUI-widgets: cell_buffer, block_widget, list/table, gauge, sparkline, chart, braille_canvas
- **z381** TUI-layout: constraint_length/percentage/fill, hstack/vstack/zstack, grid, overlay, popup_center
- **z382** Farger og stil: basic16/256/RGB, style_tui (bold/dim/italic/undercurl), span_t, tui_theme (Nord/Dracula)
- **z383** Event-loop: input_parser, escape_seq_timeout, event_channel, tick_rate, tui_frame, backend_draw
- **z384** Animasjon: spinner (braille/arc/clock/moon), progress_bar med ETA, multi_progress, smooth_value, typewriter
- **z385** Interaktiv input: text_input (cursor/mask/word-jump), textarea, fuzzy_finder, select_input, file_picker
- **z386** App-rammeverk: screen_stack, component, focus_ring, keymap, command_palette, notification, modal, pane_split
- **z387** TUI-stdlib: pre-bygde apper (list/table/log/explorer/process), `nc tui pick/table/log/confirm/form/run`

### Nåværende modell etter Omgang 387

```text
Norscode TUI-stack:

Terminal:    alternate_screen + cursor_hide + raw_mode + SIGWINCH
Widgets:     cell_buffer + block_border + list + table + gauge + chart + braille
Layout:      constraint(length/pct/fill) + hstack/vstack + grid + popup_center
Farger:      basic16 / xterm-256 / RGB (truecolor) + style modifiers + tui_theme
Input:       text_input + textarea + fuzzy_finder + file_picker + form
Event-loop:  async poll + escape_timeout + tick + event_channel (async tasks)
Animasjon:   spinner(10 stiler) + progress_bar(ETA) + smooth_value + typewriter
App:         screen_stack + focus_ring + keymap + command_palette + modal + pane
stdlib:      nc tui pick / table / log / confirm / form / run / menu
```

### Neste naturlige fase etter z387

Neste store omgang bør være **databasemotor og lagringsmotor** (z388+):

1. B-tre og B+-tre indeks-implementasjon
2. WAL (Write-Ahead Logging) og durability
3. MVCC (Multi-Version Concurrency Control)
4. Spørsmål-optimizer og plan-cache
5. Transaksjonsmotor og isolasjonsnivåer
6. Lagringsformat og sidehåndtering
7. Replikering og point-in-time recovery
8. Norscode SQL-dialekt og ORM

---

## Fase 21: Databasemotor og lagringsmotor (z388–z397)

Omgang 388–397 introduserte komplett databasemotor og lagrings-infrastruktur.

Siste dokumenterte store milepæl: Omgang 397.

Kjerne:

```text
B+-tre + WAL(ARIES) + MVCC + optimizer + sidehåndtering
+ replikering + SQL-builder + ORM + migrasjoner + stdlib
```

- **z388** B-tre/B+-tre: node-split, rotering, sammenslåing, bplus_leaf_chain, btree_cursor
- **z389** WAL (ARIES): LSN, rtype_full_page, group_commit, checkpoint, Analysis→Redo→Undo recovery
- **z390** MVCC: tuple_xmin/xmax, txn_snapshot, is_visible, SSI (rw_anti_dependency), VACUUM, wraparound
- **z391** Sidehåndtering: page_header, item_id, heap_tuple, buffer_pool med clock_sweep, FSM, visibility_map
- **z392** Query optimizer: statistics (NDV, histogram), selectivity, dp join order, GEQO, access paths, plan_cache
- **z393** Replikering og PITR: streaming (walsender/receiver), synchronous_replication, logical decoding, base_backup
- **z394** SQL-dialekt og query builder: qb_from/where/join/group, eb_eq/like/in, CTE, lateral, window functions
- **z395** ORM og migrasjoner: model→tabell, rel_belongs_to/has_many/many_to_many, auto_migration, schema_diff, seeder
- **z396** Tilkoblingspool og drivere: pool_min/max, health_check, driver_postgres/sqlite/duckdb, with_timeout
- **z397** DB stdlib: with_tx, savepoints, slow_query_log, test_helper (within_tx_test), `nc db migrate/console/generate`, norscodedb (embedded)

### Nåværende modell etter Omgang 397

```text
Norscode databasemotor:

Indeks:      B+-tre med leaf_chain + btree_cursor (range scan O(log n + k))
WAL:         LSN + ARIES (Analysis/Redo/Undo) + group_commit + archiving
MVCC:        tuple_xmin/xmax + snapshot_isolation + SSI + VACUUM + wraparound
Lagring:     8KB pages + buffer_pool(clock_sweep) + free_space_map + visibility_map
Optimizer:   histogram + NDV + dynamic_programming join order + GEQO + plan_cache
Replikering: streaming (sync/async) + logical decoding + base_backup + PITR
SQL:         query_builder + expr_builder + CTE + lateral + window functions
ORM:         model → tabell + relasjoner + eager load + auto_migration
Pool:        min/max conns + health_check + ctx_timeout + slow_query_log
Test:        within_tx_test (alltid rollback) + test_snapshot_db
stdlib:      std.db.orm / std.db.sql / std.db.pool / std.db.migration
embedded:    norscodedb (WAL, in-memory, ingen ekstern server)
CLI:         nc db migrate / rollback / status / console / schema / diff / generate
```

### Neste naturlige fase etter z397

Neste store omgang bør være **kompilatorkjerne og bytekode-VM** (z398+):

1. Bytekode-instruksjonssett og opcode-definisjon
2. Stakk-basert VM med call frames
3. Innebygde funksjoner (builtins) i VM
4. Garbage collector for VM-heap
5. JIT-kompilering av varme funksjoner
6. Debugger-støtte i VM (breakpoints, step)
7. Profilering og ytelsestelling
8. VM-snapshot og serialisering

---

## Fase 22: VM-kjerne fortsettelse (z398–z407)

z398–z407 fullførte VM-kjernen og modulsystemet.

- **z398** Bytekode-instruksjonssett: encode_abc/abx, fullt opcode-vokabular
- **z399** Stakk-VM: call frames, closures, upvalues, constant pool
- **z400** Builtins: I/O, liste/map/tekst, matte, fil, JSON, assert
- **z401** GC: tri-color mark-sweep, inkrementell, generational
- **z402** JIT: type-profilering, trace-opptak, OSR, deoptimering
- **z403** Debugger: betingede breakpoints, step-modi, frame-inspeksjon
- **z404** Profiler: sampling, flame graph, call graph
- **z405** Snapshot: inkrementell heap-serialisering, format-migrering
- **z406** Modulsystem: laster, resolver, cross-module dispatch, hot reload
- **z407** Unntak: unwind + finally, handler-søk, panic, result-propagering

---

## Fase 23: Flertrådskjøring og async/await runtime (z408–z417)

Omgang 408–417 introduserte komplett concurrency- og async-infrastruktur.

Siste dokumenterte store milepæl: Omgang 417.

Kjerne:

```text
green_threads + channels + async/await + reactor + mutex/atomic
+ structured_concurrency + async_io + async_net + async_fs + runtime
```

- **z408** Green threads: cooperative scheduling, context switch, work stealing
- **z409** Channels: buffered/unbuffered, select med fair scheduling, fan-out/in
- **z410** Async/await: future-trait, waker, state machine desugaring, join_all/race
- **z411** I/O reactor: epoll/kqueue/io_uring, timer wheel, async read/write
- **z412** Mutex/RWLock/atomics: mutex_guard, condvar, CAS, memory ordering, spin_lock
- **z413** Structured concurrency: task_group, cancel_token, nursery, deadline
- **z414** Async streams og sinks: combinatorer, buffered I/O, framing, json_lines
- **z415** Async TCP/UDP: connect/accept/listen, split_stream, dns_lookup, timeouts
- **z416** Async filsystem: open/read/write/seek, dir ops, file watching, spawn_blocking
- **z417** Async runtime stdlib: multi-thread executor, sleep/interval, broadcast/watch/mpsc, `nc async`

### Nåværende modell etter Omgang 417

```text
Norscode concurrency-stack:

Grunnlag:    green_thread + stack_g + context_switch + work_steal
Kommunikasjon: channel (buffered/unbuffered) + select + oneshot
Async:       future + waker + state_machine + await_expr
Reaktor:     epoll/kqueue/io_uring + timer_wheel + io_source
Synkronisering: mutex + rwlock + condvar + atomic (CAS, ordering)
Struktur:    task_group + cancel_token + nursery + deadline
Strømmer:    async_stream + async_sink + buf_reader + framed
Nettverk:    async_tcp + async_udp + dns_lookup_a
Filsystem:   async_file + dir_read + watch_a + spawn_blocking
Runtime:     multi_thread executor + sleep + broadcast/watch/mpsc
stdlib:      std.async.io/net/fs/sync/time/task
CLI:         nc async run / bench / trace
```

### Neste naturlige fase etter z417

Neste store omgang bør være **kodegenerering og native backend** (z418+):

1. IR → AArch64 instruksjonsvalg for full Norscode-delmengde
2. Funksjonskall-konvensjon (ABI) og stakkhåndtering
3. Strenghåndtering og heap-allokering i native
4. Liste og map-operasjoner i native
5. Kontrollflyt: alle betingede og ubetingede hopp
6. Modulkall og ekstern symboloppløsning
7. Debug-info og kildekart for native binær
8. Kompiler `selfhost/vm.no` til native macOS-binær

---

## Fase 24: Kodegenerering og native backend (z418–z427)

Omgang 418–427 definerer komplett native kompileringspipeline og bootstrap-fullføring.

Siste dokumenterte store milepæl: Omgang 427.

- **z418** Instruksjonsvalg: BURS tiling, virtuelle registre, cmp+branch fusjon
- **z419** ABI: AArch64 macOS/Linux kallekonvensjon, stakk-layout, prologue/epilogue
- **z420** Verdirepr: tagged pointer, inline strings, alloc_header, RC
- **z421** Liste/map native: ring_buffer, robin_hood hashing, builtins
- **z422** Kontrollflyt: jump table, exception table, landing pad, unwind
- **z423** Modulkobling: symbol_table, GOT/PLT, dlopen, module ctors
- **z424** Debug-info: DWARF, line table, di_subprogram, dSYM
- **z425** Runtime-bibliotek: rt_alloc/str/list/map/io/error/gc
- **z426** Full pipeline: parse → typecheck → SSA → isel → regalloc → emit → link
- **z427** Bootstrap-fullføring: stage A/B/C, replace_python_wrapper, python_free_check

### Neste naturlige fase etter z427

Neste store omgang bør være **standardbibliotek nativ implementasjon** (z428+):

1. `std.tekst` nativ — split, replace, trim, format, regex
2. `std.liste` nativ — sort, map, filter, fold, zip
3. `std.ordbok` nativ — merge, filter, transform
4. `std.fil` nativ — path, read, write, walk
5. `std.json` nativ — parse, stringify med full typestøtte
6. `std.math` nativ — aritmetikk, trigonometri, statistikk
7. `std.tid` nativ — now, format, parse, duration
8. `std.env` nativ — args, getenv, platform

---

## Fase 25: Standardbibliotek nativ implementasjon (z428–z437)

Omgang 428–437 introduserte komplett nativ standardbibliotek for Norscode.

Siste dokumenterte store milepæl: Omgang 437.

```text
std.tekst + std.liste + std.ordbok + std.fil + std.json
+ std.math + std.tid + std.env + std.mønster + stdlib-integrasjon
```

- **z428** `std.tekst`: split/join, trim, søk/erstatt, formater, case, Unicode-norm
- **z429** `std.liste`: map/filter/fold/skann, zip, grupper, vinduer, biter, sorter, sett-ops
- **z430** `std.ordbok` + `std.fil` + `std.path`: map-transform, path-join, fil-les/skriv, mappetraversering
- **z431** `std.json`: rekursiv-descent parser, serialisering, JSON Pointer, schema-validering, streaming
- **z432** `std.math`: abs/min/max/klamp, sqrt/exp/log/trig, statistikk, random, interpolasjon
- **z433** `std.tid`: tidspunkt/dato/klokkeslett/dato_tid, tidssone, varighet, formatering, sov/vent
- **z434** `std.env` + `std.prosess` + `std.plattform`: args, env-vars, prosess-spawn, signaler, CPU/minne
- **z435** `std.mønster`: Thompson NFA → DFA, capture-grupper, erstatt_funksjon, glob
- **z436** `std.kanal` + `std.logg` + `std.feil`: pub/sub, signal-hooks, roterende logg, rik feiltype
- **z437** Stdlib-integrasjon: modul-registry, lazy/eager loading, compat-sjekk, property-testing

### Nåværende modell etter Omgang 437

```text
Norscode standardbibliotek (nativt):

std.tekst   — split/join/trim/søk/erstatt/formater/Unicode
std.liste   — map/filter/fold/grupper/vinduer/sorter/sett
std.ordbok  — transform/filtrer/slå_sammen/inverter
std.fil     — les/skriv/kopier/flytt/slett + mappe-traversering
std.path    — join/normalisér/kanonisér/komponenter
std.json    — parse/stringify/pointer/schema/stream
std.math    — arith/trig/log/statistikk/random
std.tid     — tidspunkt/dato/varighet/sov/formater
std.env     — args/getenv/setenv
std.prosess — spawn/vent/rør/signal
std.plattform — os/arch/cpu/minne
std.mønster — regex(NFA→DFA)/glob
std.kanal   — kanal_ny/pub_sub/signal_hook
std.logg    — strukturert logging/roterende filer
std.feil    — rik feiltype med kontekst-kjede
```

---

## Fase 26: Web-rammeverk og HTTP-server (z438–z442)

Omgang 438–442 introduserte komplett web-rammeverk for Norscode.

Siste dokumenterte store milepæl: Omgang 442.

```text
HTTP-ruter + JSON API + autentisering + statiske filer + WebSocket + nc serve
```

- **z438** HTTP-ruter og middleware-pipeline: app, rute, middleware-kjede, router_tre, prefiks-gruppe, cors, rate-limit
- **z439** JSON API og innholdsforhandling: json_api, validering, skjema, paginator, api_versjon, openapi-spec
- **z440** Autentisering og autorisasjon: jwt, sesjon, oauth2, rbac, tillatelse, csrf, api_nøkkel
- **z441** Statiske filer, maler og SSR: statiske_filer, template_motor, komponent, hydreringspunkt, SSR-cache
- **z442** WebSocket og `nc serve`: ws_rute, ws_meldinger, produksjon-server, tls_terminering, health_check

### Nåværende modell etter Omgang 442

```text
Norscode web-rammeverk:

HTTP-lag     — ruter/middleware/cors/rate-limit
JSON API     — validering/schema/paginator/openapi
Auth         — jwt/sesjon/oauth2/rbac/csrf
Statisk/SSR  — template/komponent/hydration/cache
WebSocket    — ws_rute/ws_meldinger/pub_sub
nc serve     — produksjon/tls/health/graceful-shutdown
```

---

## Fase 27: Spillmotor og grafikk (z443–z452)

Omgang 443–452 introduserte komplett spillmotor for Norscode.

Siste dokumenterte store milepæl: Omgang 452.

```text
spill_løkke + ECS + 2D/3D-rendering + fysikk + inndata + scene + AI + nettverk + nc spill
```

- **z443** Spill-løkke og livssyklus: spill_løkke, fast_tidssteg, fps_begrenset, tilstand_maskin, spill_klokke
- **z444** ECS-arkitektur: entitet, komponent, system, arketype, bue_spørring, system_plan, parallell_system
- **z445** 2D-rendering og sprites: sprite, sprite_blad, animasjon, tilkart, rendring_lag, sprite_batcher
- **z446** Fysikk og kollisjonsdeteksjon: stivkropp, kollisjons_form, SAT-kollisjonstesting, impuls, kvadtre
- **z447** Inndata, lyd og ressursadministrasjon: tastatur, mus, spillkontroller, lyd_kilde, ressurs_laster
- **z448** Sceneliste og hierarki: scene_tre, scene_node, transform, kamera, lys, prefab, instansiering
- **z449** 3D-rendering og shadere: nett, PBR-materiale, skyggelegger_uniformer, render_kø, etterbehandling
- **z450** Banesøking og AI: A_stjar, navigasjonsnett, mengde_simulering, atferd_tre, AI_agent
- **z451** Flerspiller-nettverk: rollback_netcode, snapshot_delta, matchmaking, lag_kompensasjon, elo
- **z452** Spillmotor-stdlib: innebygde komponenter/systemer, `nc spill`-kommandoer, spill_template

### Nåværende modell etter Omgang 452

```text
Norscode spillmotor:

Kjerne       — spill_løkke/tilstand_maskin/fast_tidssteg
ECS          — entitet/komponent/system/arketype/bue_spørring
2D           — sprite/animasjon/tilkart/fysikk2d/kollisjons_detektor
3D           — nett/PBR/skyggelegger/render_kø/etterbehandling
Inndata      — tastatur/mus/spillkontroller/berøring
Lyd          — lyd_kilde/3D-lyd/miksepanel/strøm
Ressurser    — ressurs_laster/asset-pakke/asynkron-laster
Scene        — scene_tre/prefab/kamera/lys/instansiering
AI           — A*/navigasjonsnett/mengde/atferd_tre
Nettverk     — rollback/snapshot_delta/matchmaking/elo
nc spill     — spill_ny/spill_kjør/legg_til_scene/publiser
```

---

## Fase 28: Editor-verktøy og DevEx (z453–z457)

Omgang 453–457 introduserte komplett editor-infrastruktur for Norscode.

- **z453** Scene-editor: hierarki, komponent-inspektør, transform-gizmos, undo/redo, prefab
- **z454** Visuell debugger og profiler: FPS-diagram, system-profil, render/fysikk/nett-debugger
- **z455** Hot-reload: fil-vakt, tilstandsbevaring, shader-fallback, scene-diff
- **z456** Asset-pipeline: bilde/lyd/3D-behandler, mipmap/LOD/atlas, pakkeformat
- **z457** Plugin-system og nc studio: utvidelsespunkter, sandbox, dockbar IDE med LSP

---

## Fase 29: Skripting, Playground, testing, l10n og telemetri (z458–z462)

Omgang 458–462 dekket utviklerverktøy og internasjonalisering.

- **z458** Skripting-sandkasse: isolert VM, ressurskvoter, hot-patch, live REPL, WASM-sandbox
- **z459** Norscode Playground: nettleser-REPL, notatbok-modell, deling, innebygde eksempler
- **z460** Spilltesting: enhetstester, simulasjonstest, fuzzing, egenskapstest, benchmark
- **z461** Lokalisering: kataloger, flertallsregler, bidi-tekst, pseudolokalisering
- **z462** Analytikk og krasj-rapportering: samtykke, hendelser, trakt, behold, offline-buffer

---

## Fase 30: Sky, tilgjengelighet, byggesystem, monetisering og sosiale funksjoner (z463–z467)

Omgang 463–467 dekket plattform og forretningsfunksjoner.

- **z463** Sky-deploy: container-bygg, rullende/blå-grønn/kanarie, auto-skalering, spillserverpool
- **z464** Tilgjengelighet (a11y): skjermleser, semantisk tre, fargekontrast, bevegelsesreduksjon
- **z465** Byggesystem: inkrementell kompilering, parallell bygg, kryssplattform, prosjektmaler
- **z466** Monetisering: IAP, abonnementer, kvitteringsvalidering, virtuell valuta, belønningsannonser
- **z467** Sosiale funksjoner: topplister, prestasjoner, venner, tilstedeværelse, aktivitetsstrøm

---

## Fase 31: Moderasjon, lagring, prosedyregenerering, shadere og plattformintegrasjon (z468–z472)

Omgang 468–472 dekket innholdssikkerhet og avansert rendering.

- **z468** Innholdsmoderasjon: tekst/bilde-screening, rapportsystem, anti-juks, foreldrekontroll
- **z469** Lagringssystem: lokal/sky-backend, auto-lagring, konfliktløsning, skjemaversjonering
- **z470** Prosedyrebasert generering: støyfunksjoner, BSP, WFC, L-systemer, navngenerering
- **z471** Avansert shader-system: kryssbackend-kompilering, post-process stack (bloom/DoF/SSAO)
- **z472** Plattformintegrasjon: varslinger, dype lenker, widgets, bakgrunnshenting, termisk tilstand

---

## Fase 32: Tilstand, animasjon, DSP, VR, ORM, makroer, terreng, ML, observerbarhet og flagg (z473–z482)

Omgang 473–482 er den største enkeltfasen — ti filer som dekker kjerneinfrastruktur.

Siste dokumenterte store milepæl: Omgang 482.

- **z473** Reaktiv tilstandsadministrasjon: signaler, avledet, effekter, reduserer, optimistiske oppdateringer
- **z474** Animasjonssystem: tweening, lettingskurver, nøkkelrammer, fjærfysikk, lag-blanding
- **z475** Audio DSP: nodegrafer, oscillatorer, ADSR, filtre, reverb/delay, granulær, steg-sekvenser
- **z476** VR/AR: OpenXR, stereo-rendering, foveation, håndsporing, romlige ankere, AR-plan-detektor
- **z477** ORM: modell/felt/relasjoner, spørringsbygger, transaksjoner, migrasjoner, tilkoblingsbasseng
- **z478** Makrosystem: prosedyremakroer, avlede-makroer, deklarative mønstre, kompileringstidsevaluering
- **z479** Terrengrendering: chunked LOD, clipmap, splat-teksturering, GPU-vegetasjon, hydraulisk erosjon
- **z480** ML-inferens: ONNX/GGUF/SafeTensors, kvantisering, LLM med KV-buffer, objektdeteksjon
- **z481** Observerbarhet: OpenTelemetry-sporing, metrikker, strukturert logging, hale-basert sampling
- **z482** Konfigurasjon og funksjonsflagg: lagdelte kilder, hemmeligheter, målretting, gradvis utrulling, A/B

### Nåværende modell etter Omgang 482

```text
Norscode runtime-dekning (z001–z482):

Kjerne-VM       — bytekode, GC, typer, unntak, moduler
Kompilator      — parser, typesjekker, optimizer, native-backend (AArch64)
Stdlib          — tekst/liste/ordbok/fil/json/math/tid/env/mønster/kanal/logg
Async/konk      — løfter, oppgaver, kanaler, aktører, STM, reaktive strømmer
Nettverk        — TCP/UDP/HTTP/WebSocket/gRPC/QUIC/DNS
Web             — ruter/middleware/JSON API/auth/SSR/WebSocket/nc serve
Spillmotor      — spill_løkke/ECS/2D-3D-rendering/fysikk/lyd/AI/nettverk
Editor          — scene-editor/profiler/hot-reload/asset-pipeline/nc studio
Prosedyre       — støy/BSP/WFC/L-system/navngenerering/erosjon
Tilstand        — signaler/reduserer/effekter/tilstandsmaskiner
Animasjon       — tween/nøkkelrammer/fjær/bane/sekvenser
DSP             — nodegrafer/filtre/reverb/granulær/steg-sekvenser
VR/AR           — OpenXR/stereo/foveation/håndsporing/romlige ankere
ORM             — modell/spørringsbygger/transaksjoner/migrasjoner
Makroer         — prosedyre/avlede/deklarativ/kompileringstid
Terreng         — LOD/clipmap/splat/vegetasjon/erosjon
ML              — ONNX/GGUF/kvantisering/LLM/deteksjon
Observerbarhet  — OTel-sporing/metrikker/logg/hale-sampling
Flagg           — funksjonsflagg/målretting/A-B/gradvis utrulling
Sky             — container/auto-skalering/serverhosting/IaC
Plattform       — varslinger/dype lenker/widgets/termisk
Tilgjengelighet — skjermleser/kontrast/bevegelse/teksting
Sikkerhet       — krypto/TLS/auth/RBAC/anti-juks/moderasjon
```

---

## Fase 33: Distribuerte systemer og databehandling (z483–z492)

Omgang 483–492 dekket kjerne-infrastruktur for distribuerte systemer.

- **z483** Event sourcing og CQRS: hendelseslager, projeksjoner, prosessledere, optimistisk låsing
- **z484** Saga-mønster: orkestrering/koreografi, kompenserende transaksjoner, 2PC, TCC, utboks
- **z485** Meldingskøer: Kafka (produsent/forbruker/KTable), NATS JetStream, RabbitMQ exchange-topologi
- **z486** GraphQL: skjema, resolvere, DataLoader (N+1), abonnementer, klient-cache, kodegenerering
- **z487** Reaktive strømmer: kald/varm observable, flatMap/switchMap, debounce, emner, planleggere
- **z488** Data-pipeline/ETL: DAG-kjøring, inkrementell ekstraksjon, validering, karantene, kontrollpunkt
- **z489** Strøm-aggregering: tumle/skyve/økt-vinduer, vannmerker, sen data, HyperLogLog, strøm-SQL
- **z490** Distribuert koordinering: leder-valg, Raft-konsensus, distribuerte låser, Redlock, gjerdingstoken
- **z491** Skjemaregister: Avro/Protobuf, kompatibilitetsnivåer, datakontraktstesting, evolusjonsstrategi
- **z492** CDC: PostgreSQL WAL/MySQL binlog, før/etter-bilder, full+inkrementell sync, feltsensurering

---

## Fase 34: Søk, IAM, arbeidsflyt, GIS, TSDB, grafdb, vektordb, agenter, IR og innebygd (z493–z502)

Omgang 493–502 dekket spesialiserte databaser og infrastruktur.

- **z493** Søkemotor: invertert indeks, BM25, bool-spørringer, aggregeringer, fuzzy, hybrid vektor+tekst
- **z494** IAM: OIDC/PKCE, SAML-påstander, MFA/FIDO2/TOTP, ABAC-policyer, JIT-provisjonering
- **z495** Arbeidsflyt-motor: varig utførelse via hendelseslogg-replay, aktiviteter, signaler, parallellflyt
- **z496** GIS: koordinatsystemer, romlige operatorer, R-tree/H3-indeksering, CH-ruting, vektorfliser
- **z497** Tidsserie-DB: LSM-tre, Gorilla-komprimering, nedsampling, kontinuerlige spørringer, PromQL
- **z498** Graf-DB: node/kant-modell, traversal, korteste sti, PageRank, Louvain, Cypher/GQL
- **z499** Vektor-DB: HNSW, IVF-PQ, filtrert søk, hybrid BM25+vektor, MMR, binær kvantisering
- **z500** Fleragens-systemer: verktøybruk, flerlagminne, ReAct, refleksjon, orkestrator/underagent
- **z501** Kompilator-mellomende: SSA-IR, dominanstre, konstant-folding/propagering, sløyfe-opt, inlining
- **z502** Innebygd system: oppstartsekvens, GPIO/UART/SPI/I2C, avbrudd, PWM, strømstyring, RTOS

---

## Fase 35: Blokkjede, CV, NLP, P2P, formell, IoT, robotikk, simulering, OS og kvanteberegning (z503–z512)

Omgang 503–512 dekket avanserte domener fra krypto til kvantefysikk.

- **z503** Blokkjede: blokker, transaksjoner, HD-lommebøker, smarte kontrakter, PoS, ERC20/721
- **z504** Datamaskin-syn: kantdeteksjon, ORB-egenskaper, YOLO-deteksjon, SAM-segmentering, ansiktsgjenkjenning
- **z505** NLP: tokenisering, POS-tagging, avhengighetstre, NER, koferens, sentiment, dialogsystem
- **z506** P2P-nettverk: Kademlia DHT, CID-innholdsadressering, GossipSub, yamux, NAT-traversal
- **z507** Formell verifisering: avhengige typer, lineære typer, effektsystem, CTL/LTL, SMT-løser
- **z508** Digital tvilling/IoT: MQTT/CoAP/LoRaWAN, ønsket/rapportert tilstand, kantberegning
- **z509** Robotikk: DH-kinematikk, IK-Jacobian, RRT*, SLAM med EKF, MPC-styring, ROS2
- **z510** Simuleringsmotor: Monte Carlo, kvasitilfeldige sekvenser, agentbasert, diskret hendelse
- **z511** OS-primitiver: prosess-spawn, mutex/rwlokk, inotify, mmap, io_uring, null-kopi
- **z512** Kvanteberegning: qubit-tilstander, porter, QFT, Grover, Shor, VQE, feilkorreksjon

---

## Fase 36: Numerisk, vitenskapelig, finans, frontend, minnemodell, kodek, bio, verktøy, netsim, regalloc (z513–z522)

Omgang 513–522 dekket dyp teknisk infrastruktur på tvers av alle fagfelt.

Siste dokumenterte store milepæl: Omgang 522.

- **z513** Numerisk beregning: BLAS/LAPACK, matrise-dekomponering (LU/QR/SVD/Chol), FFT, sparse
- **z514** Vitenskapelig beregning: RK45/Rosenbrock ODE, finite differanser/elementer for PDE
- **z515** Finansiell beregning: Black-Scholes, greske bokstaver, rentekurve, VaR/CVaR, Markowitz
- **z516** Kompilator-frontend: lekser, Pratt-parser, inkrementell re-parsing, rike diagnostikker
- **z517** Minnemodell: SC/acq-rel/avslappet, låsfri kø/stabel, hazard-pekere, epochbasert GC
- **z518** Audio/video-kodek: H.264/H.265/AV1, AAC/Opus, MP4/MKV, transcode-pipeline, HLS/DASH
- **z519** Bioinformatikk: Needleman-Wunsch, BLAST, BWA-justering, variantanrop, RNA-seq
- **z520** Språkverktøy: formaterer med importsortering, linter med syklomatisk kompleksitet, full REPL
- **z521** Nettverksimulering: OSPF/BGP, CoDel-køer, TCP BBR-analyse, kaos-nett med feilinjeksjon
- **z522** Registerallokering og instruksjonsplanlegging: lineær scan, graffarging med koalesering, listeplanlegger

### Nåværende modell etter Omgang 522

```text
Norscode runtime-dekning (z001–z522):

Kjerne-VM          — bytekode, GC, typer, unntak, moduler, minnemodell
Kompilator         — frontend (lekser/Pratt-parser/inkrementell), IR/SSA, opt-passes,
                     registerallokering (lineær scan/graffarging), instruksjonsplanlegging,
                     AArch64/WASM-backend
Stdlib             — tekst/liste/ordbok/fil/json/math/tid/env/regex
Async/konk         — løfter, oppgaver, kanaler, aktører, STM, reaktive strømmer (Rx)
Nettverk           — TCP/UDP/HTTP/WebSocket/gRPC/QUIC + nettverksimulering
Web                — ruter/middleware/JSON API/auth/SSR/WebSocket/nc serve
Spillmotor         — spill_løkke/ECS/2D-3D/fysikk/lyd/AI/nettverk/nc spill
Editor             — scene-editor/profiler/hot-reload/asset-pipeline/nc studio
Distribuert        — event sourcing/CQRS/saga/Kafka/GraphQL/CDC/koordinering
Databaser          — ORM/søkemotor/tidsserie/grafdb/vektordb/skjemaregister
Prosedyre          — støy/BSP/WFC/L-system/navngenerering/erosjon
Tilstand/UI        — signaler/reduserer/animasjon/tweening/fjærfysikk
DSP/Media          — nodegrafer/filtre/reverb/granulær + H.264/AV1/AAC/Opus/HLS
VR/AR              — OpenXR/stereo/foveation/håndsporing/romlige ankere
Numerisk           — BLAS/LAPACK/FFT/sparse + ODE/PDE + finansiell + bioinformatikk
ML/AI              — ONNX/GGUF/kvantisering/LLM + fleragens/ReAct/verktøybruk
Observerbarhet     — OTel-sporing/metrikker/logg + funksjonsflagg/A-B
Sky/Plattform      — container/skalering/spillserver/a11y/IAM/krypto
Innebygd/OS        — GPIO/UART/mmap/io_uring + RTOS + kvanteberegning
Verktøy            — formaterer/linter/REPL/byggesystem/prosjektmaler/playground
```

---

## Fase 37: Typeteori, JIT, GC, LLVM, profiler, FFI, dokumentasjon, register og testing (z523–z532)

Omgang 523–532 fullførte kjerneinfrastrukturen for Norscode som selvstendig plattform.

Siste dokumenterte store milepæl: Omgang 532.

- **z523** Algebraiske effekter og ADT: typede effektrekker, delimited continuations, uttømmende mønster-matching
- **z524** Typeklasse-system: Functor/Monad/Semigroup, høyere-kinded typer, dictionary-passing
- **z525** JIT-kompilering: nivåbasert utførelse, spekulativ inlining, inline-cacher, deoptimisering
- **z526** Søppelsamler: generasjons-GC, trifarget invariant, write-barriere, concurrent mark, safepoints
- **z527** LLVM-integrasjon: komplett IR, alle instruksjoner, SROA/GVN/LICM/vektorisering-passes
- **z528** Minneprofiler: allokerings-sporing, heap-snapshot, retensjonstrær, lekkasjepåvisning
- **z529** Kryssplatform-FFI: C (dlopen/dlsym), Rust (cbindgen/uniffi), Python (pyo3/NumPy), JS (V8/N-API)
- **z530** Dokumentasjonsgenerator: doc-kommentarer, kjørbare doctests, søkeindeks, versjonert publisering
- **z531** Pakkeregister: PubGrub-løsning, lockfil, Sigstore-signering, CVE-revisjon, SBOM
- **z532** Generativ testing: typede generatorer, krymping, tilstandsbasert modell-testing, dekningsrettet fuzzing

### Nåværende modell etter Omgang 532

```text
Norscode kjerne (z001–z532) — fullstendig plattformdekning:

Typesystem        — ADT/sum-typer, avhengige typer, lineære typer, effektrekker,
                    typeklasser (Eq/Ord/Hash/Functor/Monad), HKT, algebraiske effekter
Kompilator        — frontend (lekser/Pratt/inkrementell), IR/SSA, opt-passes,
                    registerallokering, instruksjonsplanlegging, LLVM-backend, JIT
Runtime           — generasjons-GC (trifarget/concurrent), minnemodell (SC/acq-rel),
                    safepoints, finalizers, svake referanser, minneprofiler
Stdlib            — komplett standardbibliotek + FFI (C/Rust/Python/JS)
Verktøy           — formaterer, linter, REPL, LSP, docs-generator, pakkeregister
Testing           — enhet, integrasjon, simulasjon, generativ, fuzzing, egenskap-bevis
Distribuert       — event sourcing, saga, Kafka, GraphQL, CDC, koordinering, schema
Databaser         — ORM, søk, tidsserie, grafdb, vektordb, dokumentdb
Systemer          — OS-primitiver, innebygd (GPIO/UART/RTOS), kvanteberegning
Spesialisert      — numerisk, vitenskapelig, finans, bioinformatikk, CV, NLP, GIS
Plattform         — spillmotor, VR/AR, IoT, robotikk, blokkjede, sky
```

---

## Fase 38: Avanserte språkfunksjoner og metaprogrammering (z533–z542)

Omgang 533–542 fullførte det teoretiske grunnlaget for Norscode som et avansert programmeringsspråk.

Siste dokumenterte store milepæl: Omgang 542.

- **z533** Avansert makro-hygiene: quasi-quotation, def/call-site spans, derive-motor med struct/enum-data
- **z534** Bevis-assistert programmering: Curry-Howard, pi/sigma-typer, taktikker (simp/omega/aesop), ekstraksjon
- **z535** Kompileringstids-beregning: konst-funksjoner, konst-generikk, forhåndsberegnede oppslagstabeller
- **z536** Funktor-modularsystem: ML-signaturer/strukturer, opake typer, generative/applikative funktorer
- **z537** Inkrementell kompilering: Salsa spørringssystem, API/impl-hashing, parallell kompilering
- **z538** Distribuert bygg: innholdsadressert cache, hermetiske sandkasser, reproduserbare bygg
- **z539** Spesialiseringsbasert kodegen: monomorfe instansiering, delvis evaluering, polytypisme
- **z540** Profileringsstyrt optimalisering: instrumentering, gren-vektlegging, AutoFDO, BOLT
- **z541** Gradert typeteori: lineære/afine ressurser, eierskap, levetider, kapabiliteter, sesjonstyper
- **z542** Effekt-polymorfisme: rad-type-inferens, HM med effektbegrensninger, rad-unifikasjon

### Nåværende modell etter Omgang 542

```text
Norscode fullstendig dekning (z001–z542):

Typeteori          — ADT, avhengige typer, lineære typer, graderte typer,
                    effektrekker (row), typeklasser, HKT, bevistyper (Curry-Howard)
Kompilator-kjerne  — inkrementell (Salsa), parallell, distribuert (CAS-cache),
                    LLVM/AArch64-backend, JIT (tiered), PGO, spesialisering
Runtime            — GC (generasjons/concurrent), minnemodell, JIT-deopt,
                    minneprofiler, safepoints
Modularsystem      — ML-funktor-moduler, signaturer, opake typer
Metaprogrammering  — hygienske makroer, quasi-quote, derive, konst-evaluering
Bevis              — Curry-Howard, taktikker, terminering, ekstraksjon
[... alle tidligere domenener fra Fase 1–37 ...]
```

### Neste naturlige fase etter z542

Neste store omgang bør dekke **avansert kjøretidssystemer og plattformmodning** (z543+):

1. Persisteringsrammeverk (append-only log, snapshot-restore)
2. Sikkerhets-sandkasse og capability-VM
3. Språkserver-protokoll (LSP) full implementasjon
4. Multippel-retur og unstrukturert kontrollflyt
5. Heterogen beregning (GPU-kjerne-programmering)
6. Programsyntese og skisse-basert programmering
7. Adaptiv runtime-optimalisering
8. Minnekomprimering og heap-compaction
9. Nettverks-namespaces og container-runtime
10. Formell semantikk og operasjonell semantikk
