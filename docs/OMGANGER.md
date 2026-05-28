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
