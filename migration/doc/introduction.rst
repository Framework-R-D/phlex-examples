Introduction
============

This guide provides practical guidance for migrating code that uses the event processing framework, *art* to *Phlex*.
Before code can be expressed naturally in *Phlex*, it often needs to be simplified and reorganized.
The main difference in style is that *art* modules are typically written around an event-processing loop owned by the framework, while *Phlex* encourages a
more functional style: explicit inputs, explicit outputs, and algorithms that can be executed independently of framework state.
For that reason, this guide is organized around the steps that make that transition manageable.

The guide has four parts:

1. ``Preparation`` identifies *art* constructs that should be removed, isolated, or reshaped before migration.
2. ``Refactoring`` separates framework-dependent code from algorithm code.
3. ``Mapping`` shows how a refactored *art* design is expressed in *Phlex* concepts.
4. ``Putting It All Together`` walks through the migration flow end to end.

This guide assumes that the reader is familiar with existing *art* code and is trying to move an established code base toward a *Phlex*-compatible structure.
It does not assume that the original code was written with migration in mind.

The guide also assumes that migration may happen in stages.
In many cases, the best first step is not to write *Phlex* code immediately, but to make the existing *art* code easier to reason about.
That preparation work often yields simpler, more testable code even before the final migration is complete.

Because many existing code bases have limited test coverage, some migration work will require careful manual validation.
The examples in this guide therefore aim to make behavior-preserving refactoring as explicit as possible.

A useful way to read this guide is:

1. identify which *art* concepts appear in the current code,
2. perform the preparation work for those concepts,
3. refactor the code so the algorithmic core is isolated, and only then
4. map the remaining framework boundary into *Phlex*.

That order matters.
Attempting to map heavily framework-entangled *art* code straight into *Phlex* usually produces awkward interfaces and preserves unnecessary complexity.
