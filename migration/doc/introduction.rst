Introduction
============

This guide provides practical guidance for migrating code from the
event-processing framework *art* to *Phlex*. Before code can be expressed
naturally in *Phlex*, it often needs to be simplified and reorganized.

*art* modules are typically written around an event-processing loop owned by
the framework. *Phlex* encourages a different style: algorithms with clear
inputs and outputs that can run independently of framework state. Reaching that
style usually requires preparation work before any *Phlex* code is written.

The guide has three parts:

1. **Framework and Domain Logic Separation** identifies *art* constructs that
   should be removed, isolated, or reshaped before migration.
2. **Algorithm Extraction** refactors the code so the algorithm is defined in
   plain functions or small classes with no framework code in the algorithm
   itself.
3. **Phlex Binding** shows how an extracted design is expressed in *Phlex*
   concepts.

That order matters. Attempting to map heavily framework-entangled *art* code
straight into *Phlex* usually produces awkward interfaces and preserves
unnecessary complexity.

Throughout this guide, the **framework boundary** refers to the code that
directly interacts with the framework: configuration lookup, input retrieval,
service access, registration, and output publication. The goal of migration is
to confine those operations to that boundary and keep the algorithm itself as
ordinary C++.

This guide assumes the reader is familiar with existing *art* code and is
working with an established code base that was not written with migration in
mind. It also assumes migration may happen in stages. Separation and extraction
work often yields simpler, more testable code even before *Phlex* code is
written.

Because many existing code bases have limited test coverage, some migration
work will require careful manual validation. The examples aim to make
behavior-preserving algorithm extraction as explicit as possible.
