# V7600 Callsite vs function body

## Formål
Dette steget skil mellom:
- liste-literal i kallstaden
- liste bygga via ei grøn hjelpebane (`builtin.split`)
- sjølve `bygg_bundle(...)`

## Prober
- `list_arg_local_fn_v7600.no`
- `list_var_local_fn_v7600.no`
- `bundler_single_module_split_v7600.no`

## Tolkning
- dersom lokal funksjon med liste-literal feilar, er callsite/listeliteral i aktiv compile-path framleis mistenkt
- dersom lokal funksjon med split-lista verkar, er det sterk støtte for at liste-literal-kallstaden er problemet
- dersom `bygg_bundle(split(...))` framleis feilar, sit problemet i funksjonen eller vidare nedstraums
