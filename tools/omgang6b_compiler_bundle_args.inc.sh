# tools/omgang6b_compiler_bundle_args.inc.sh — delt modulliste for Omgang 6b stage-0
# Source frå build_omgang6b_compiler_ncb.sh og selfcompile_stage0_elf.sh
OMGANG6B_BUNDLE_ARGS=(
    selfhost.lexer.lexer_m1=selfhost/lexer/lexer_m1.no
    selfhost.parser=selfhost/parser.no
    selfhost.compiler.semantic=selfhost/compiler/semantic.no
    selfhost.compiler.ir_to_bytecode=selfhost/compiler/ir_to_bytecode.no
    selfhost.json=selfhost/json.no
    selfhost.kompiler=selfhost/kompiler.no
    selfhost.bundler=selfhost/bundler.no
    selfhost.elf_compile_driver=selfhost/elf_compile_driver.no
)
