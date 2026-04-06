# REAL COMPILER V35

- Hot-path acceleration for selfhost compiler helper functions in the bytecode VM.
- Bypasses expensive repeated interpretation of `normaliser_norsk_token`, `stack_behov`, `stack_endring`, `op_krever_arg`, `op_kjent`, `er_heltall_token`, and `operator_til_opcode`.
- Keeps semantics but speeds up `selfhost-chain-run tests/test_selfhost.no`.
