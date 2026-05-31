/* Auto-generert av selfhost/gen_dispatch.no — IKKJE REDIGER */
typedef struct { const char *name; NcVal *(*fn)(NcVal **, int); } NcDispatch;

static NcVal *nc_fn___main___er_bokstav(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_tall(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_ident_tegn(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_whitespace(NcVal **args, int nargs);
static NcVal *nc_fn___main___keyword_type_for(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_keyword(NcVal **args, int nargs);
static NcVal *nc_fn___main___token_type_for_ident(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_operator_start(NcVal **args, int nargs);
static NcVal *nc_fn___main___operator_type_for(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_punctuation(NcVal **args, int nargs);
static NcVal *nc_fn___main___punctuation_type(NcVal **args, int nargs);
static NcVal *nc_fn___main___deltekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___lag_token(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_ident(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_hex_tegn(NcVal **args, int nargs);
static NcVal *nc_fn___main___hex_verdi(NcVal **args, int nargs);
static NcVal *nc_fn___main___hex_tekst_til_heltall(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_tall(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___unescape_tegn(NcVal **args, int nargs);
static NcVal *nc_fn___main___unescape_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_operator(NcVal **args, int nargs);
static NcVal *nc_fn___main___hopp_kommentar(NcVal **args, int nargs);
static NcVal *nc_fn___main___lex(NcVal **args, int nargs);
static NcVal *nc_fn___main___start(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_node(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_state(NcVal **args, int nargs);
static NcVal *nc_fn___main___token_verdi(NcVal **args, int nargs);
static NcVal *nc_fn___main___token_linje(NcVal **args, int nargs);
static NcVal *nc_fn___main___token_kolonne(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_slutt(NcVal **args, int nargs);
static NcVal *nc_fn___main___gjeldende(NcVal **args, int nargs);
static NcVal *nc_fn___main___se_frem(NcVal **args, int nargs);
static NcVal *nc_fn___main___neste(NcVal **args, int nargs);
static NcVal *nc_fn___main___parserfeil(NcVal **args, int nargs);
static NcVal *nc_fn___main___matcher(NcVal **args, int nargs);
static NcVal *nc_fn___main___forvent(NcVal **args, int nargs);
static NcVal *nc_fn___main___legg_barn(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_type_token(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_parser(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_program(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_struktur(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_modul_del(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_bruk(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_test(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_type_token_type(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_type_token(NcVal **args, int nargs);
static NcVal *nc_fn___main___les_namn_token(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_parameterliste(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_blokk(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_klammeblokk(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_setning(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_la(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_returner(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_hvis(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_match(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_mens(NcVal **args, int nargs);
static NcVal *nc_fn___main___presedens(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_for(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_pr__v_fang(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_binop(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_unar(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_postfix(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_argumentliste(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_primar(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_lambda(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_fun_lambda(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_liste_literal(NcVal **args, int nargs);
static NcVal *nc_fn___main___parse_map_eller_struct_literal(NcVal **args, int nargs);
static NcVal *nc_fn___main___ast_til_snapshot(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_builtin(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_symbol_tabell(NcVal **args, int nargs);
static NcVal *nc_fn___main___registrer_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___funksjon_finst(NcVal **args, int nargs);
static NcVal *nc_fn___main___legg_til_feil(NcVal **args, int nargs);
static NcVal *nc_fn___main___samle_funksjonar(NcVal **args, int nargs);
static NcVal *nc_fn___main___sjekk_kall_i_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn___main___analyser_program(NcVal **args, int nargs);
static NcVal *nc_fn___main___hent_funksjon_param(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_escape_tegn(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_escape_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_liste(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_liste_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_instruksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_instruksjon_med_int(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_instruksjon_med_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_instruksjon_med_bool(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_kall(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_instruksjon_uten_arg(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_bool(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_fra_tekstliste(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_fra_map_parliste(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_type_til_schema(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_type_til_eksempel(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_normaliser_sti(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_kombiner_prefiks(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_split_spec(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_metode_liten(NcVal **args, int nargs);
static NcVal *nc_fn___main___route_felt_liste(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_rute_til_json(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_fra_ruteliste(NcVal **args, int nargs);
static NcVal *nc_fn___main___felt_kjede_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___tekst_ender_med(NcVal **args, int nargs);
static NcVal *nc_fn___main___web_kall_namn(NcVal **args, int nargs);
static NcVal *nc_fn___main___er_web_kall(NcVal **args, int nargs);
static NcVal *nc_fn___main___samla_web_annotasjonar(NcVal **args, int nargs);
static NcVal *nc_fn___main___route_felt_tekst(NcVal **args, int nargs);
static NcVal *nc_fn___main___samanfatt_rute(NcVal **args, int nargs);
static NcVal *nc_fn___main___emit_lambda_hjelpefunksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_komp(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_etikett(NcVal **args, int nargs);
static NcVal *nc_fn___main___push_loop(NcVal **args, int nargs);
static NcVal *nc_fn___main___pop_loop(NcVal **args, int nargs);
static NcVal *nc_fn___main___gjeldande_loop_start(NcVal **args, int nargs);
static NcVal *nc_fn___main___gjeldande_loop_slutt(NcVal **args, int nargs);
static NcVal *nc_fn___main___binop_til_opcode(NcVal **args, int nargs);
static NcVal *nc_fn___main___unar_til_opcode(NcVal **args, int nargs);
static NcVal *nc_fn___main___aug_til_binop(NcVal **args, int nargs);
static NcVal *nc_fn___main___emit_uttrykk(NcVal **args, int nargs);
static NcVal *nc_fn___main___emit_sett_verdi(NcVal **args, int nargs);
static NcVal *nc_fn___main___emit_setning(NcVal **args, int nargs);
static NcVal *nc_fn___main___emit_blokk(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_test(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_program(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_til_ncb_json(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_fil(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_fil_til_disk(NcVal **args, int nargs);
static NcVal *nc_fn___main___r__yk_test(NcVal **args, int nargs);
static NcVal *nc_fn___main___kompiler_sj__lvtest(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_ny(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_p(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_peek(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_les_ch(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_ws(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_streng(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_tal(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_verdi(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_les(NcVal **args, int nargs);
static NcVal *nc_fn___main___json_skriv(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_rammeverk(NcVal **args, int nargs);
static NcVal *nc_fn___main___ramme_les_var(NcVal **args, int nargs);
static NcVal *nc_fn___main___ramme_sett_var(NcVal **args, int nargs);
static NcVal *nc_fn___main___stack_push(NcVal **args, int nargs);
static NcVal *nc_fn___main___stack_pop(NcVal **args, int nargs);
static NcVal *nc_fn___main___stack_topp(NcVal **args, int nargs);
static NcVal *nc_fn___main___bygg_label_kart(NcVal **args, int nargs);
static NcVal *nc_fn___main___ny_try_post(NcVal **args, int nargs);
static NcVal *nc_fn___main___kall_innebygd(NcVal **args, int nargs);
static NcVal *nc_fn___main___finn_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___k__yr_funksjon(NcVal **args, int nargs);
static NcVal *nc_fn___main___k__yr_ncb(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_kompiler(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_k__yr_json(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_selftest(NcVal **args, int nargs);
static NcVal *nc_fn___main___omd__yp_funksjonar(NcVal **args, int nargs);
static NcVal *nc_fn___main___hent_funksjonar_json(NcVal **args, int nargs);
static NcVal *nc_fn___main___bygg_bundle(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_kompiler_fil(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_k__yr_ncb_json(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_k__yr_ncb_med_bootstrap(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_run(NcVal **args, int nargs);
static NcVal *nc_fn___main___nc_compile(NcVal **args, int nargs);
static NcVal *nc_fn___main___sanitiser(NcVal **args, int nargs);

static NcDispatch nc_dispatch[] = {
  {"__main__.er_bokstav", nc_fn___main___er_bokstav},
  {"__main__.er_tall", nc_fn___main___er_tall},
  {"__main__.er_ident_tegn", nc_fn___main___er_ident_tegn},
  {"__main__.er_whitespace", nc_fn___main___er_whitespace},
  {"__main__.keyword_type_for", nc_fn___main___keyword_type_for},
  {"__main__.er_keyword", nc_fn___main___er_keyword},
  {"__main__.token_type_for_ident", nc_fn___main___token_type_for_ident},
  {"__main__.er_operator_start", nc_fn___main___er_operator_start},
  {"__main__.operator_type_for", nc_fn___main___operator_type_for},
  {"__main__.er_punctuation", nc_fn___main___er_punctuation},
  {"__main__.punctuation_type", nc_fn___main___punctuation_type},
  {"__main__.deltekst", nc_fn___main___deltekst},
  {"__main__.lag_token", nc_fn___main___lag_token},
  {"__main__.les_ident", nc_fn___main___les_ident},
  {"__main__.er_hex_tegn", nc_fn___main___er_hex_tegn},
  {"__main__.hex_verdi", nc_fn___main___hex_verdi},
  {"__main__.hex_tekst_til_heltall", nc_fn___main___hex_tekst_til_heltall},
  {"__main__.les_tall", nc_fn___main___les_tall},
  {"__main__.les_tekst", nc_fn___main___les_tekst},
  {"__main__.unescape_tegn", nc_fn___main___unescape_tegn},
  {"__main__.unescape_tekst", nc_fn___main___unescape_tekst},
  {"__main__.les_operator", nc_fn___main___les_operator},
  {"__main__.hopp_kommentar", nc_fn___main___hopp_kommentar},
  {"__main__.lex", nc_fn___main___lex},
  {"__main__.start", nc_fn___main___start},
  {"__main__.ny_node", nc_fn___main___ny_node},
  {"__main__.ny_state", nc_fn___main___ny_state},
  {"__main__.token_verdi", nc_fn___main___token_verdi},
  {"__main__.token_linje", nc_fn___main___token_linje},
  {"__main__.token_kolonne", nc_fn___main___token_kolonne},
  {"__main__.er_slutt", nc_fn___main___er_slutt},
  {"__main__.gjeldende", nc_fn___main___gjeldende},
  {"__main__.se_frem", nc_fn___main___se_frem},
  {"__main__.neste", nc_fn___main___neste},
  {"__main__.parserfeil", nc_fn___main___parserfeil},
  {"__main__.matcher", nc_fn___main___matcher},
  {"__main__.forvent", nc_fn___main___forvent},
  {"__main__.legg_barn", nc_fn___main___legg_barn},
  {"__main__.er_type_token", nc_fn___main___er_type_token},
  {"__main__.ny_parser", nc_fn___main___ny_parser},
  {"__main__.parse_program", nc_fn___main___parse_program},
  {"__main__.parse_struktur", nc_fn___main___parse_struktur},
  {"__main__.les_modul_del", nc_fn___main___les_modul_del},
  {"__main__.parse_bruk", nc_fn___main___parse_bruk},
  {"__main__.parse_funksjon", nc_fn___main___parse_funksjon},
  {"__main__.parse_test", nc_fn___main___parse_test},
  {"__main__.er_type_token_type", nc_fn___main___er_type_token_type},
  {"__main__.les_type_token", nc_fn___main___les_type_token},
  {"__main__.les_namn_token", nc_fn___main___les_namn_token},
  {"__main__.parse_parameterliste", nc_fn___main___parse_parameterliste},
  {"__main__.parse_blokk", nc_fn___main___parse_blokk},
  {"__main__.parse_klammeblokk", nc_fn___main___parse_klammeblokk},
  {"__main__.parse_setning", nc_fn___main___parse_setning},
  {"__main__.parse_la", nc_fn___main___parse_la},
  {"__main__.parse_returner", nc_fn___main___parse_returner},
  {"__main__.parse_hvis", nc_fn___main___parse_hvis},
  {"__main__.parse_match", nc_fn___main___parse_match},
  {"__main__.parse_mens", nc_fn___main___parse_mens},
  {"__main__.presedens", nc_fn___main___presedens},
  {"__main__.parse_for", nc_fn___main___parse_for},
  {"__main__.parse_prøv_fang", nc_fn___main___parse_pr__v_fang},
  {"__main__.parse_uttrykk", nc_fn___main___parse_uttrykk},
  {"__main__.parse_binop", nc_fn___main___parse_binop},
  {"__main__.parse_unar", nc_fn___main___parse_unar},
  {"__main__.parse_postfix", nc_fn___main___parse_postfix},
  {"__main__.parse_argumentliste", nc_fn___main___parse_argumentliste},
  {"__main__.parse_primar", nc_fn___main___parse_primar},
  {"__main__.parse_lambda", nc_fn___main___parse_lambda},
  {"__main__.parse_fun_lambda", nc_fn___main___parse_fun_lambda},
  {"__main__.parse_liste_literal", nc_fn___main___parse_liste_literal},
  {"__main__.parse_map_eller_struct_literal", nc_fn___main___parse_map_eller_struct_literal},
  {"__main__.ast_til_snapshot", nc_fn___main___ast_til_snapshot},
  {"__main__.er_builtin", nc_fn___main___er_builtin},
  {"__main__.ny_symbol_tabell", nc_fn___main___ny_symbol_tabell},
  {"__main__.registrer_funksjon", nc_fn___main___registrer_funksjon},
  {"__main__.funksjon_finst", nc_fn___main___funksjon_finst},
  {"__main__.legg_til_feil", nc_fn___main___legg_til_feil},
  {"__main__.samle_funksjonar", nc_fn___main___samle_funksjonar},
  {"__main__.sjekk_kall_i_uttrykk", nc_fn___main___sjekk_kall_i_uttrykk},
  {"__main__.analyser_program", nc_fn___main___analyser_program},
  {"__main__.hent_funksjon_param", nc_fn___main___hent_funksjon_param},
  {"__main__.json_escape_tegn", nc_fn___main___json_escape_tegn},
  {"__main__.json_escape_tekst", nc_fn___main___json_escape_tekst},
  {"__main__.json_liste", nc_fn___main___json_liste},
  {"__main__.json_liste_tekst", nc_fn___main___json_liste_tekst},
  {"__main__.json_instruksjon", nc_fn___main___json_instruksjon},
  {"__main__.json_instruksjon_med_int", nc_fn___main___json_instruksjon_med_int},
  {"__main__.json_instruksjon_med_tekst", nc_fn___main___json_instruksjon_med_tekst},
  {"__main__.json_instruksjon_med_bool", nc_fn___main___json_instruksjon_med_bool},
  {"__main__.json_kall", nc_fn___main___json_kall},
  {"__main__.json_instruksjon_uten_arg", nc_fn___main___json_instruksjon_uten_arg},
  {"__main__.json_bool", nc_fn___main___json_bool},
  {"__main__.json_fra_tekstliste", nc_fn___main___json_fra_tekstliste},
  {"__main__.json_fra_map_parliste", nc_fn___main___json_fra_map_parliste},
  {"__main__.web_type_til_schema", nc_fn___main___web_type_til_schema},
  {"__main__.web_type_til_eksempel", nc_fn___main___web_type_til_eksempel},
  {"__main__.web_normaliser_sti", nc_fn___main___web_normaliser_sti},
  {"__main__.web_kombiner_prefiks", nc_fn___main___web_kombiner_prefiks},
  {"__main__.web_split_spec", nc_fn___main___web_split_spec},
  {"__main__.web_metode_liten", nc_fn___main___web_metode_liten},
  {"__main__.route_felt_liste", nc_fn___main___route_felt_liste},
  {"__main__.web_rute_til_json", nc_fn___main___web_rute_til_json},
  {"__main__.json_fra_ruteliste", nc_fn___main___json_fra_ruteliste},
  {"__main__.felt_kjede_tekst", nc_fn___main___felt_kjede_tekst},
  {"__main__.tekst_ender_med", nc_fn___main___tekst_ender_med},
  {"__main__.web_kall_namn", nc_fn___main___web_kall_namn},
  {"__main__.er_web_kall", nc_fn___main___er_web_kall},
  {"__main__.samla_web_annotasjonar", nc_fn___main___samla_web_annotasjonar},
  {"__main__.route_felt_tekst", nc_fn___main___route_felt_tekst},
  {"__main__.samanfatt_rute", nc_fn___main___samanfatt_rute},
  {"__main__.emit_lambda_hjelpefunksjon", nc_fn___main___emit_lambda_hjelpefunksjon},
  {"__main__.ny_komp", nc_fn___main___ny_komp},
  {"__main__.ny_etikett", nc_fn___main___ny_etikett},
  {"__main__.push_loop", nc_fn___main___push_loop},
  {"__main__.pop_loop", nc_fn___main___pop_loop},
  {"__main__.gjeldande_loop_start", nc_fn___main___gjeldande_loop_start},
  {"__main__.gjeldande_loop_slutt", nc_fn___main___gjeldande_loop_slutt},
  {"__main__.binop_til_opcode", nc_fn___main___binop_til_opcode},
  {"__main__.unar_til_opcode", nc_fn___main___unar_til_opcode},
  {"__main__.aug_til_binop", nc_fn___main___aug_til_binop},
  {"__main__.emit_uttrykk", nc_fn___main___emit_uttrykk},
  {"__main__.emit_sett_verdi", nc_fn___main___emit_sett_verdi},
  {"__main__.emit_setning", nc_fn___main___emit_setning},
  {"__main__.emit_blokk", nc_fn___main___emit_blokk},
  {"__main__.kompiler_funksjon", nc_fn___main___kompiler_funksjon},
  {"__main__.kompiler_test", nc_fn___main___kompiler_test},
  {"__main__.kompiler_program", nc_fn___main___kompiler_program},
  {"__main__.kompiler_til_ncb_json", nc_fn___main___kompiler_til_ncb_json},
  {"__main__.kompiler_fil", nc_fn___main___kompiler_fil},
  {"__main__.kompiler_fil_til_disk", nc_fn___main___kompiler_fil_til_disk},
  {"__main__.røyk_test", nc_fn___main___r__yk_test},
  {"__main__.kompiler_sjølvtest", nc_fn___main___kompiler_sj__lvtest},
  {"__main__.json_ny", nc_fn___main___json_ny},
  {"__main__.json_p", nc_fn___main___json_p},
  {"__main__.json_peek", nc_fn___main___json_peek},
  {"__main__.json_les_ch", nc_fn___main___json_les_ch},
  {"__main__.json_ws", nc_fn___main___json_ws},
  {"__main__.json_streng", nc_fn___main___json_streng},
  {"__main__.json_tal", nc_fn___main___json_tal},
  {"__main__.json_verdi", nc_fn___main___json_verdi},
  {"__main__.json_les", nc_fn___main___json_les},
  {"__main__.json_skriv", nc_fn___main___json_skriv},
  {"__main__.ny_rammeverk", nc_fn___main___ny_rammeverk},
  {"__main__.ramme_les_var", nc_fn___main___ramme_les_var},
  {"__main__.ramme_sett_var", nc_fn___main___ramme_sett_var},
  {"__main__.stack_push", nc_fn___main___stack_push},
  {"__main__.stack_pop", nc_fn___main___stack_pop},
  {"__main__.stack_topp", nc_fn___main___stack_topp},
  {"__main__.bygg_label_kart", nc_fn___main___bygg_label_kart},
  {"__main__.ny_try_post", nc_fn___main___ny_try_post},
  {"__main__.kall_innebygd", nc_fn___main___kall_innebygd},
  {"__main__.finn_funksjon", nc_fn___main___finn_funksjon},
  {"__main__.køyr_funksjon", nc_fn___main___k__yr_funksjon},
  {"__main__.køyr_ncb", nc_fn___main___k__yr_ncb},
  {"__main__.nc_kompiler", nc_fn___main___nc_kompiler},
  {"__main__.nc_køyr_json", nc_fn___main___nc_k__yr_json},
  {"__main__.nc_selftest", nc_fn___main___nc_selftest},
  {"__main__.omdøyp_funksjonar", nc_fn___main___omd__yp_funksjonar},
  {"__main__.hent_funksjonar_json", nc_fn___main___hent_funksjonar_json},
  {"__main__.bygg_bundle", nc_fn___main___bygg_bundle},
  {"__main__.nc_kompiler_fil", nc_fn___main___nc_kompiler_fil},
  {"__main__.nc_køyr_ncb_json", nc_fn___main___nc_k__yr_ncb_json},
  {"__main__.nc_køyr_ncb_med_bootstrap", nc_fn___main___nc_k__yr_ncb_med_bootstrap},
  {"__main__.nc_run", nc_fn___main___nc_run},
  {"__main__.nc_compile", nc_fn___main___nc_compile},
  {"__main__.sanitiser", nc_fn___main___sanitiser},
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
