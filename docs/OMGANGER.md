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
