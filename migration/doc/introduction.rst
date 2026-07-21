Introduction
============

This guide provides practical guidance for migrating code from the event-processing framework *art* to *Phlex*.
Before code can be expressed naturally in *Phlex*, it often needs to be simplified and reorganized.
The main difference in style is that *art* modules are typically written around an event-processing loop owned by the framework, while *Phlex* encourages algorithms with clear inputs and outputs that can run independently of framework state.
For that reason, this guide is organized around the steps that make that transition manageable.

The guide has three parts:  

1. ``Framework and Domain Logic Separation`` identifies *art* constructs that should be removed, isolated, or reshaped before migration.
2. ``Algorithm Extraction`` refactors the code so the algorithm is defined in plain functions or small classes with explicit inputs and outputs, without framework code in the algorithm itself.
3. ``Phlex Binding`` shows how an extracted *art* design is expressed in *Phlex* concepts.

This guide assumes that the reader is familiar with existing *art* code and is trying to move an established code base toward a *Phlex*-compatible structure.
It does not assume that the original code was written with migration in mind.

In this guide, the framework boundary is the code that directly interacts with
the framework: configuration lookup, input retrieval, service access,
registration, and output publication. The goal of migration is to confine those
operations to that boundary and keep the algorithm itself as ordinary C++.

The guide also assumes that migration may happen in stages.
In many cases, the best first step is not to write *Phlex* code immediately, but to make the existing *art* code easier to reason about.
That separation work often yields simpler, more testable code before the final migration is complete.

Because many existing code bases have limited test coverage, some migration work will require careful manual validation.
The examples in this guide therefore aim to make behavior-preserving algorithm extraction into framework-independent code as explicit as possible.

The main idea is:

1. identify which *art* concepts appear in the current code,
2. perform the framework and domain logic separation work for those concepts,
3. extract the algorithm into framework-independent functions or classes, and only then
4. bind the remaining framework-boundary code into *Phlex*.

That order matters.
Attempting to map heavily framework-entangled *art* code straight into *Phlex* usually produces awkward interfaces and preserves unnecessary complexity.
