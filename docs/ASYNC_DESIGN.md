# Async Design

This document locks the initial async direction for Norscode.

## Goal

Add a small, understandable async model that works well for IO-heavy code without making the language feel heavy.

## Chosen model

- `async` functions compile to resumable state machines.
- `await` suspends the current function until the awaited task is ready.
- Tasks are cooperative, not preemptive.
- Cancellation is explicit through a token or handle.
- Timeouts are modeled as ordinary task wrappers instead of special syntax.

## Why this shape

- It keeps the syntax close to Python while still being easy to lower in the compiler.
- It fits the existing compiler structure: parser -> semantic -> runtime -> codegen.
- It allows a first implementation that can start with a small task runtime and grow later.

## Initial scope

- `async` function declarations
- `await` expressions
- a basic task/future runtime
- explicit timeout helpers
- explicit cancellation helpers

## Out of scope for the first step

- preemptive scheduling
- thread pools as the primary model
- complex cancellation trees
- implicit magic around IO

## Acceptance shape

The first version is good enough when:

- an async function can call another async function and await its result
- timeout and cancellation failures remain readable
- the model stays simple enough to reason about in tests
