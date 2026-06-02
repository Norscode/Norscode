/* Auto-generert av selfhost/gen_dispatch.no — IKKJE REDIGER */
typedef struct { const char *name; NcVal *(*fn)(NcVal **, int); } NcDispatch;

static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_bokstav(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_tall(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_ident_tegn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_whitespace(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_keyword_type_for(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_keyword(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_token_type_for_ident(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_operator_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_operator_type_for(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_punctuation(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_punctuation_type(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_deltekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_lag_token(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_les_ident(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_er_hex_tegn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_hex_verdi(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_hex_tekst_til_heltall(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_les_tall(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_les_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_unescape_tegn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_unescape_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_les_operator(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_hopp_kommentar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_lex(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_lexer_lexer_m1_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_ny_node(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_ny_state(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_token_verdi(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_token_linje(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_token_kolonne(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_er_slutt(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_gjeldende(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_se_frem(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_neste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parserfeil(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_matcher(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_forvent(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_legg_barn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_er_type_token(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_ny_parser(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_program(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_struktur(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_les_modul_del(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_bruk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_test(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_er_type_token_type(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_les_type_token(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_les_namn_token(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_parameterliste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_blokk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_klammeblokk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_setning(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_la(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_returner(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_hvis(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_match(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_mens(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_presedens(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_for(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_pr__v_fang(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_binop(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_unar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_postfix(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_argumentliste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_primar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_lambda(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_fun_lambda(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_liste_literal(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_parse_map_eller_struct_literal(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_parser_ast_til_snapshot(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_er_builtin(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_ny_symbol_tabell(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_registrer_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_funksjon_finst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_legg_til_feil(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_samle_funksjonar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_sjekk_kall_i_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_analyser_program(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_hent_funksjon_param(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_semantic_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_escape_tegn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_escape_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_liste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_liste_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_int(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_bool(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_kall(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_uten_arg(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_bool(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_tekstliste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_map_parliste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_type_til_schema(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_type_til_eksempel(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_normaliser_sti(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_kombiner_prefiks(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_split_spec(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_metode_liten(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_route_felt_liste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_rute_til_json(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_ruteliste(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_felt_kjede_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_tekst_ender_med(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_web_kall_namn(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_er_web_kall(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_samla_web_annotasjonar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_route_felt_tekst(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_samanfatt_rute(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_emit_lambda_hjelpefunksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_ny_komp(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_ny_etikett(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_push_loop(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_pop_loop(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_gjeldande_loop_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_gjeldande_loop_slutt(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_binop_til_opcode(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_unar_til_opcode(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_aug_til_binop(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_emit_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_emit_sett_verdi(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_emit_setning(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_emit_blokk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_test(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_program(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_compiler_ir_to_bytecode_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_kompiler_fil(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_kompiler_fil_til_disk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_r__yk_test(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_kompiler_sj__lvtest(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_kompiler_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_ny(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_p(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_peek(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_les_ch(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_ws(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_streng(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_tal(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_verdi(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_les(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_json_json_skriv(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_ny_rammeverk(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_ramme_les_var(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_ramme_sett_var(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_stack_push(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_stack_pop(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_stack_topp(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm__rt_map_n__kler(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm__rt_har_n__kkel(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_bygg_label_kart(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_ny_try_post(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_kall_innebygd(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_vm_sikker_get(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_finn_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_vm_opcode_fra_instr(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_k__yr_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_k__yr_ncb(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_vm_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_bundler_omd__yp_funksjonar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_bundler_sl___saman_funksjonar(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_bundler_bygg_bundle(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_bundler_start(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_nc_kompiler_fil(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_nc_k__yr_ncb_json(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_nc_run(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_nc_compile(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_nc_l5b_gen2(NcVal **args, int nargs);
static NcVal *nc_fn_selfhost_nc_main_start(NcVal **args, int nargs);

static NcDispatch nc_dispatch[] = {
  {"selfhost.lexer.lexer_m1.er_bokstav", nc_fn_selfhost_lexer_lexer_m1_er_bokstav},
  {"selfhost.lexer.lexer_m1.er_tall", nc_fn_selfhost_lexer_lexer_m1_er_tall},
  {"selfhost.lexer.lexer_m1.er_ident_tegn", nc_fn_selfhost_lexer_lexer_m1_er_ident_tegn},
  {"selfhost.lexer.lexer_m1.er_whitespace", nc_fn_selfhost_lexer_lexer_m1_er_whitespace},
  {"selfhost.lexer.lexer_m1.keyword_type_for", nc_fn_selfhost_lexer_lexer_m1_keyword_type_for},
  {"selfhost.lexer.lexer_m1.er_keyword", nc_fn_selfhost_lexer_lexer_m1_er_keyword},
  {"selfhost.lexer.lexer_m1.token_type_for_ident", nc_fn_selfhost_lexer_lexer_m1_token_type_for_ident},
  {"selfhost.lexer.lexer_m1.er_operator_start", nc_fn_selfhost_lexer_lexer_m1_er_operator_start},
  {"selfhost.lexer.lexer_m1.operator_type_for", nc_fn_selfhost_lexer_lexer_m1_operator_type_for},
  {"selfhost.lexer.lexer_m1.er_punctuation", nc_fn_selfhost_lexer_lexer_m1_er_punctuation},
  {"selfhost.lexer.lexer_m1.punctuation_type", nc_fn_selfhost_lexer_lexer_m1_punctuation_type},
  {"selfhost.lexer.lexer_m1.deltekst", nc_fn_selfhost_lexer_lexer_m1_deltekst},
  {"selfhost.lexer.lexer_m1.lag_token", nc_fn_selfhost_lexer_lexer_m1_lag_token},
  {"selfhost.lexer.lexer_m1.les_ident", nc_fn_selfhost_lexer_lexer_m1_les_ident},
  {"selfhost.lexer.lexer_m1.er_hex_tegn", nc_fn_selfhost_lexer_lexer_m1_er_hex_tegn},
  {"selfhost.lexer.lexer_m1.hex_verdi", nc_fn_selfhost_lexer_lexer_m1_hex_verdi},
  {"selfhost.lexer.lexer_m1.hex_tekst_til_heltall", nc_fn_selfhost_lexer_lexer_m1_hex_tekst_til_heltall},
  {"selfhost.lexer.lexer_m1.les_tall", nc_fn_selfhost_lexer_lexer_m1_les_tall},
  {"selfhost.lexer.lexer_m1.les_tekst", nc_fn_selfhost_lexer_lexer_m1_les_tekst},
  {"selfhost.lexer.lexer_m1.unescape_tegn", nc_fn_selfhost_lexer_lexer_m1_unescape_tegn},
  {"selfhost.lexer.lexer_m1.unescape_tekst", nc_fn_selfhost_lexer_lexer_m1_unescape_tekst},
  {"selfhost.lexer.lexer_m1.les_operator", nc_fn_selfhost_lexer_lexer_m1_les_operator},
  {"selfhost.lexer.lexer_m1.hopp_kommentar", nc_fn_selfhost_lexer_lexer_m1_hopp_kommentar},
  {"selfhost.lexer.lexer_m1.lex", nc_fn_selfhost_lexer_lexer_m1_lex},
  {"selfhost.lexer.lexer_m1.start", nc_fn_selfhost_lexer_lexer_m1_start},
  {"selfhost.parser.ny_node", nc_fn_selfhost_parser_ny_node},
  {"selfhost.parser.ny_state", nc_fn_selfhost_parser_ny_state},
  {"selfhost.parser.token_verdi", nc_fn_selfhost_parser_token_verdi},
  {"selfhost.parser.token_linje", nc_fn_selfhost_parser_token_linje},
  {"selfhost.parser.token_kolonne", nc_fn_selfhost_parser_token_kolonne},
  {"selfhost.parser.er_slutt", nc_fn_selfhost_parser_er_slutt},
  {"selfhost.parser.gjeldende", nc_fn_selfhost_parser_gjeldende},
  {"selfhost.parser.se_frem", nc_fn_selfhost_parser_se_frem},
  {"selfhost.parser.neste", nc_fn_selfhost_parser_neste},
  {"selfhost.parser.parserfeil", nc_fn_selfhost_parser_parserfeil},
  {"selfhost.parser.matcher", nc_fn_selfhost_parser_matcher},
  {"selfhost.parser.forvent", nc_fn_selfhost_parser_forvent},
  {"selfhost.parser.legg_barn", nc_fn_selfhost_parser_legg_barn},
  {"selfhost.parser.er_type_token", nc_fn_selfhost_parser_er_type_token},
  {"selfhost.parser.ny_parser", nc_fn_selfhost_parser_ny_parser},
  {"selfhost.parser.parse_program", nc_fn_selfhost_parser_parse_program},
  {"selfhost.parser.parse_struktur", nc_fn_selfhost_parser_parse_struktur},
  {"selfhost.parser.les_modul_del", nc_fn_selfhost_parser_les_modul_del},
  {"selfhost.parser.parse_bruk", nc_fn_selfhost_parser_parse_bruk},
  {"selfhost.parser.parse_funksjon", nc_fn_selfhost_parser_parse_funksjon},
  {"selfhost.parser.parse_test", nc_fn_selfhost_parser_parse_test},
  {"selfhost.parser.er_type_token_type", nc_fn_selfhost_parser_er_type_token_type},
  {"selfhost.parser.les_type_token", nc_fn_selfhost_parser_les_type_token},
  {"selfhost.parser.les_namn_token", nc_fn_selfhost_parser_les_namn_token},
  {"selfhost.parser.parse_parameterliste", nc_fn_selfhost_parser_parse_parameterliste},
  {"selfhost.parser.parse_blokk", nc_fn_selfhost_parser_parse_blokk},
  {"selfhost.parser.parse_klammeblokk", nc_fn_selfhost_parser_parse_klammeblokk},
  {"selfhost.parser.parse_setning", nc_fn_selfhost_parser_parse_setning},
  {"selfhost.parser.parse_la", nc_fn_selfhost_parser_parse_la},
  {"selfhost.parser.parse_returner", nc_fn_selfhost_parser_parse_returner},
  {"selfhost.parser.parse_hvis", nc_fn_selfhost_parser_parse_hvis},
  {"selfhost.parser.parse_match", nc_fn_selfhost_parser_parse_match},
  {"selfhost.parser.parse_mens", nc_fn_selfhost_parser_parse_mens},
  {"selfhost.parser.presedens", nc_fn_selfhost_parser_presedens},
  {"selfhost.parser.parse_for", nc_fn_selfhost_parser_parse_for},
  {"selfhost.parser.parse_prøv_fang", nc_fn_selfhost_parser_parse_pr__v_fang},
  {"selfhost.parser.parse_uttrykk", nc_fn_selfhost_parser_parse_uttrykk},
  {"selfhost.parser.parse_binop", nc_fn_selfhost_parser_parse_binop},
  {"selfhost.parser.parse_unar", nc_fn_selfhost_parser_parse_unar},
  {"selfhost.parser.parse_postfix", nc_fn_selfhost_parser_parse_postfix},
  {"selfhost.parser.parse_argumentliste", nc_fn_selfhost_parser_parse_argumentliste},
  {"selfhost.parser.parse_primar", nc_fn_selfhost_parser_parse_primar},
  {"selfhost.parser.parse_lambda", nc_fn_selfhost_parser_parse_lambda},
  {"selfhost.parser.parse_fun_lambda", nc_fn_selfhost_parser_parse_fun_lambda},
  {"selfhost.parser.parse_liste_literal", nc_fn_selfhost_parser_parse_liste_literal},
  {"selfhost.parser.parse_map_eller_struct_literal", nc_fn_selfhost_parser_parse_map_eller_struct_literal},
  {"selfhost.parser.ast_til_snapshot", nc_fn_selfhost_parser_ast_til_snapshot},
  {"selfhost.compiler.semantic.er_builtin", nc_fn_selfhost_compiler_semantic_er_builtin},
  {"selfhost.compiler.semantic.ny_symbol_tabell", nc_fn_selfhost_compiler_semantic_ny_symbol_tabell},
  {"selfhost.compiler.semantic.registrer_funksjon", nc_fn_selfhost_compiler_semantic_registrer_funksjon},
  {"selfhost.compiler.semantic.funksjon_finst", nc_fn_selfhost_compiler_semantic_funksjon_finst},
  {"selfhost.compiler.semantic.legg_til_feil", nc_fn_selfhost_compiler_semantic_legg_til_feil},
  {"selfhost.compiler.semantic.samle_funksjonar", nc_fn_selfhost_compiler_semantic_samle_funksjonar},
  {"selfhost.compiler.semantic.sjekk_kall_i_uttrykk", nc_fn_selfhost_compiler_semantic_sjekk_kall_i_uttrykk},
  {"selfhost.compiler.semantic.analyser_program", nc_fn_selfhost_compiler_semantic_analyser_program},
  {"selfhost.compiler.semantic.hent_funksjon_param", nc_fn_selfhost_compiler_semantic_hent_funksjon_param},
  {"selfhost.compiler.semantic.start", nc_fn_selfhost_compiler_semantic_start},
  {"selfhost.compiler.ir_to_bytecode.json_escape_tegn", nc_fn_selfhost_compiler_ir_to_bytecode_json_escape_tegn},
  {"selfhost.compiler.ir_to_bytecode.json_escape_tekst", nc_fn_selfhost_compiler_ir_to_bytecode_json_escape_tekst},
  {"selfhost.compiler.ir_to_bytecode.json_liste", nc_fn_selfhost_compiler_ir_to_bytecode_json_liste},
  {"selfhost.compiler.ir_to_bytecode.json_liste_tekst", nc_fn_selfhost_compiler_ir_to_bytecode_json_liste_tekst},
  {"selfhost.compiler.ir_to_bytecode.json_instruksjon", nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon},
  {"selfhost.compiler.ir_to_bytecode.json_instruksjon_med_int", nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_int},
  {"selfhost.compiler.ir_to_bytecode.json_instruksjon_med_tekst", nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_tekst},
  {"selfhost.compiler.ir_to_bytecode.json_instruksjon_med_bool", nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_med_bool},
  {"selfhost.compiler.ir_to_bytecode.json_kall", nc_fn_selfhost_compiler_ir_to_bytecode_json_kall},
  {"selfhost.compiler.ir_to_bytecode.json_instruksjon_uten_arg", nc_fn_selfhost_compiler_ir_to_bytecode_json_instruksjon_uten_arg},
  {"selfhost.compiler.ir_to_bytecode.json_bool", nc_fn_selfhost_compiler_ir_to_bytecode_json_bool},
  {"selfhost.compiler.ir_to_bytecode.json_fra_tekstliste", nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_tekstliste},
  {"selfhost.compiler.ir_to_bytecode.json_fra_map_parliste", nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_map_parliste},
  {"selfhost.compiler.ir_to_bytecode.web_type_til_schema", nc_fn_selfhost_compiler_ir_to_bytecode_web_type_til_schema},
  {"selfhost.compiler.ir_to_bytecode.web_type_til_eksempel", nc_fn_selfhost_compiler_ir_to_bytecode_web_type_til_eksempel},
  {"selfhost.compiler.ir_to_bytecode.web_normaliser_sti", nc_fn_selfhost_compiler_ir_to_bytecode_web_normaliser_sti},
  {"selfhost.compiler.ir_to_bytecode.web_kombiner_prefiks", nc_fn_selfhost_compiler_ir_to_bytecode_web_kombiner_prefiks},
  {"selfhost.compiler.ir_to_bytecode.web_split_spec", nc_fn_selfhost_compiler_ir_to_bytecode_web_split_spec},
  {"selfhost.compiler.ir_to_bytecode.web_metode_liten", nc_fn_selfhost_compiler_ir_to_bytecode_web_metode_liten},
  {"selfhost.compiler.ir_to_bytecode.route_felt_liste", nc_fn_selfhost_compiler_ir_to_bytecode_route_felt_liste},
  {"selfhost.compiler.ir_to_bytecode.web_rute_til_json", nc_fn_selfhost_compiler_ir_to_bytecode_web_rute_til_json},
  {"selfhost.compiler.ir_to_bytecode.json_fra_ruteliste", nc_fn_selfhost_compiler_ir_to_bytecode_json_fra_ruteliste},
  {"selfhost.compiler.ir_to_bytecode.felt_kjede_tekst", nc_fn_selfhost_compiler_ir_to_bytecode_felt_kjede_tekst},
  {"selfhost.compiler.ir_to_bytecode.tekst_ender_med", nc_fn_selfhost_compiler_ir_to_bytecode_tekst_ender_med},
  {"selfhost.compiler.ir_to_bytecode.web_kall_namn", nc_fn_selfhost_compiler_ir_to_bytecode_web_kall_namn},
  {"selfhost.compiler.ir_to_bytecode.er_web_kall", nc_fn_selfhost_compiler_ir_to_bytecode_er_web_kall},
  {"selfhost.compiler.ir_to_bytecode.samla_web_annotasjonar", nc_fn_selfhost_compiler_ir_to_bytecode_samla_web_annotasjonar},
  {"selfhost.compiler.ir_to_bytecode.route_felt_tekst", nc_fn_selfhost_compiler_ir_to_bytecode_route_felt_tekst},
  {"selfhost.compiler.ir_to_bytecode.samanfatt_rute", nc_fn_selfhost_compiler_ir_to_bytecode_samanfatt_rute},
  {"selfhost.compiler.ir_to_bytecode.emit_lambda_hjelpefunksjon", nc_fn_selfhost_compiler_ir_to_bytecode_emit_lambda_hjelpefunksjon},
  {"selfhost.compiler.ir_to_bytecode.ny_komp", nc_fn_selfhost_compiler_ir_to_bytecode_ny_komp},
  {"selfhost.compiler.ir_to_bytecode.ny_etikett", nc_fn_selfhost_compiler_ir_to_bytecode_ny_etikett},
  {"selfhost.compiler.ir_to_bytecode.push_loop", nc_fn_selfhost_compiler_ir_to_bytecode_push_loop},
  {"selfhost.compiler.ir_to_bytecode.pop_loop", nc_fn_selfhost_compiler_ir_to_bytecode_pop_loop},
  {"selfhost.compiler.ir_to_bytecode.gjeldande_loop_start", nc_fn_selfhost_compiler_ir_to_bytecode_gjeldande_loop_start},
  {"selfhost.compiler.ir_to_bytecode.gjeldande_loop_slutt", nc_fn_selfhost_compiler_ir_to_bytecode_gjeldande_loop_slutt},
  {"selfhost.compiler.ir_to_bytecode.binop_til_opcode", nc_fn_selfhost_compiler_ir_to_bytecode_binop_til_opcode},
  {"selfhost.compiler.ir_to_bytecode.unar_til_opcode", nc_fn_selfhost_compiler_ir_to_bytecode_unar_til_opcode},
  {"selfhost.compiler.ir_to_bytecode.aug_til_binop", nc_fn_selfhost_compiler_ir_to_bytecode_aug_til_binop},
  {"selfhost.compiler.ir_to_bytecode.emit_uttrykk", nc_fn_selfhost_compiler_ir_to_bytecode_emit_uttrykk},
  {"selfhost.compiler.ir_to_bytecode.emit_sett_verdi", nc_fn_selfhost_compiler_ir_to_bytecode_emit_sett_verdi},
  {"selfhost.compiler.ir_to_bytecode.emit_setning", nc_fn_selfhost_compiler_ir_to_bytecode_emit_setning},
  {"selfhost.compiler.ir_to_bytecode.emit_blokk", nc_fn_selfhost_compiler_ir_to_bytecode_emit_blokk},
  {"selfhost.compiler.ir_to_bytecode.kompiler_funksjon", nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_funksjon},
  {"selfhost.compiler.ir_to_bytecode.kompiler_test", nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_test},
  {"selfhost.compiler.ir_to_bytecode.kompiler_program", nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_program},
  {"selfhost.compiler.ir_to_bytecode.kompiler_til_ncb_json", nc_fn_selfhost_compiler_ir_to_bytecode_kompiler_til_ncb_json},
  {"selfhost.compiler.ir_to_bytecode.start", nc_fn_selfhost_compiler_ir_to_bytecode_start},
  {"selfhost.kompiler.kompiler_fil", nc_fn_selfhost_kompiler_kompiler_fil},
  {"selfhost.kompiler.kompiler_fil_til_disk", nc_fn_selfhost_kompiler_kompiler_fil_til_disk},
  {"selfhost.kompiler.røyk_test", nc_fn_selfhost_kompiler_r__yk_test},
  {"selfhost.kompiler.kompiler_sjølvtest", nc_fn_selfhost_kompiler_kompiler_sj__lvtest},
  {"selfhost.kompiler.start", nc_fn_selfhost_kompiler_start},
  {"selfhost.json.json_ny", nc_fn_selfhost_json_json_ny},
  {"selfhost.json.json_p", nc_fn_selfhost_json_json_p},
  {"selfhost.json.json_peek", nc_fn_selfhost_json_json_peek},
  {"selfhost.json.json_les_ch", nc_fn_selfhost_json_json_les_ch},
  {"selfhost.json.json_ws", nc_fn_selfhost_json_json_ws},
  {"selfhost.json.json_streng", nc_fn_selfhost_json_json_streng},
  {"selfhost.json.json_tal", nc_fn_selfhost_json_json_tal},
  {"selfhost.json.json_verdi", nc_fn_selfhost_json_json_verdi},
  {"selfhost.json.json_les", nc_fn_selfhost_json_json_les},
  {"selfhost.json.json_skriv", nc_fn_selfhost_json_json_skriv},
  {"selfhost.vm.ny_rammeverk", nc_fn_selfhost_vm_ny_rammeverk},
  {"selfhost.vm.ramme_les_var", nc_fn_selfhost_vm_ramme_les_var},
  {"selfhost.vm.ramme_sett_var", nc_fn_selfhost_vm_ramme_sett_var},
  {"selfhost.vm.stack_push", nc_fn_selfhost_vm_stack_push},
  {"selfhost.vm.stack_pop", nc_fn_selfhost_vm_stack_pop},
  {"selfhost.vm.stack_topp", nc_fn_selfhost_vm_stack_topp},
  {"selfhost.vm._rt_map_nøkler", nc_fn_selfhost_vm__rt_map_n__kler},
  {"selfhost.vm._rt_har_nøkkel", nc_fn_selfhost_vm__rt_har_n__kkel},
  {"selfhost.vm.bygg_label_kart", nc_fn_selfhost_vm_bygg_label_kart},
  {"selfhost.vm.ny_try_post", nc_fn_selfhost_vm_ny_try_post},
  {"selfhost.vm.kall_innebygd", nc_fn_selfhost_vm_kall_innebygd},
  {"selfhost.vm.vm_sikker_get", nc_fn_selfhost_vm_vm_sikker_get},
  {"selfhost.vm.finn_funksjon", nc_fn_selfhost_vm_finn_funksjon},
  {"selfhost.vm.vm_opcode_fra_instr", nc_fn_selfhost_vm_vm_opcode_fra_instr},
  {"selfhost.vm.køyr_funksjon", nc_fn_selfhost_vm_k__yr_funksjon},
  {"selfhost.vm.køyr_ncb", nc_fn_selfhost_vm_k__yr_ncb},
  {"selfhost.vm.start", nc_fn_selfhost_vm_start},
  {"selfhost.bundler.omdøyp_funksjonar", nc_fn_selfhost_bundler_omd__yp_funksjonar},
  {"selfhost.bundler.slå_saman_funksjonar", nc_fn_selfhost_bundler_sl___saman_funksjonar},
  {"selfhost.bundler.bygg_bundle", nc_fn_selfhost_bundler_bygg_bundle},
  {"selfhost.bundler.start", nc_fn_selfhost_bundler_start},
  {"selfhost.nc_main.nc_kompiler_fil", nc_fn_selfhost_nc_main_nc_kompiler_fil},
  {"selfhost.nc_main.nc_køyr_ncb_json", nc_fn_selfhost_nc_main_nc_k__yr_ncb_json},
  {"selfhost.nc_main.nc_run", nc_fn_selfhost_nc_main_nc_run},
  {"selfhost.nc_main.nc_compile", nc_fn_selfhost_nc_main_nc_compile},
  {"selfhost.nc_main.nc_l5b_gen2", nc_fn_selfhost_nc_main_nc_l5b_gen2},
  {"selfhost.nc_main.start", nc_fn_selfhost_nc_main_start},
  {"host_exec_ncb_json", nc_fn_builtin_host_exec_ncb_json},
  {"host_kall_bygg_bundle", nc_fn_builtin_host_kall_bygg_bundle},
  {NULL, NULL}
};

static NcVal *nc_dispatch_call(const char *n, NcVal **a, int na) {
    for(int i=0;nc_dispatch[i].name;i++) if(!strcmp(nc_dispatch[i].name,n)) return nc_dispatch[i].fn(a,na);
    const char *l=strrchr(n,'.');if(l)l++;else l=n;
    for(int i=0;nc_dispatch[i].name;i++){const char *fl=strrchr(nc_dispatch[i].name,'.');fl=fl?fl+1:nc_dispatch[i].name;if(!strcmp(fl,l))return nc_dispatch[i].fn(a,na);}
    if(!strncmp(n,"builtin.",8)) return nc_dispatch_call(n+8,a,na);
    if(!strncmp(n,"__main__.",9)) return nc_dispatch_call(n+9,a,na);
    {char s2[256];strncpy(s2,l,255);char *t=strstr(s2,"_token");if(t){*t=0;return nc_dispatch_call(s2,a,na);}}
    return NULL;
}
NcVal *nc_fn_builtin_neste_token(NcVal **a, int na) { return nc_dispatch_call("neste",a,na); }
