Refactoring
===========

After preparation, the next step is refactoring. The purpose of refactoring is
to separate the code into two parts:

* an algorithm part, and
* a framework-dependent part.

This is the key structural change that makes migration to *Phlex* practical.
Without it, the migration remains a translation of framework-specific code. 
With it, the migration becomes a matter of binding explicit inputs and outputs to a
well-defined computation.
The algorithm part can be tested and compared against the original implementation.
The challenge is the lack of tests in the original code, which makes it difficult to verify that the refactored algorithm is correct.
However, the refactoring process itself can be guided by the principle of minimizing changes to the core logic, while moving framework-specific code out of the algorithm and into a separate boundary layer. 
This allows for a more modular and maintainable codebase, and makes it easier to migrate to *Phlex* in the next step.

A successful refactoring produces code with the following properties.

* The algorithm can be called from ordinary C++.
* The algorithm no longer depends on module callbacks such as ``produce()`` or
  ``analyze()``.
* Configuration is gathered once and passed explicitly.
* Input products are passed in as parameters.
* Output products are returned as values.
* Framework assumptions are visible and localized.

Target Structure
----------------

The target structure is straightforward.

Algorithm part
  Contains the domain logic. This code should operate on plain C++ values,
  experiment-specific data structures, and explicitly provided helper objects.

Framework-dependent part
  Contains configuration lookup, input retrieval, helper construction,
  registration, and output publication.

The algorithm part should answer the question, "what computation is being
performed?" The framework part should answer, "how is this computation wired
into the framework?"

Recommended Refactoring Pattern
-------------------------------

A useful pattern is:

1. Define a plain configuration structure.
2. Extract the core computation into a free function or small algorithm class.
3. Pass all required inputs explicitly.
4. Return outputs directly.
5. Keep framework registration in a separate translation unit.

This pattern is intentionally simple. Most migration efforts become easier when
there are fewer abstractions, not more.

Example: Configuration as Plain Data
------------------------------------

The ``gauss_hit_finder`` example defines a plain configuration structure in
``find_hits_with_gaussians.hpp``:

.. code-block:: cpp

   struct find_hits_with_gaussians_cfg {
     bool filter_hits;
     std::vector<int> long_max_hits_vec;
     std::vector<int> long_pulse_width_vec;
     int max_multi_hit;
     int area_method;
     std::vector<double> area_norms_vec;
     double chi2_ndf;
     std::vector<float> pulse_height_cuts;
     std::vector<float> pulse_width_cuts;
     std::vector<float> pulse_ratio_cuts;
   };

This is a useful migration pattern because the structure can exist independent
of either *art* or *Phlex*. It is a normal C++ representation of the settings
that the algorithm needs.

Example: Extracted Algorithm Boundary
-------------------------------------

The same example exposes the algorithm through a normal function:

.. code-block:: cpp

   std::vector<recob::Hit>
   find_hits_with_gaussians(find_hits_with_gaussians_cfg const& cfg,
                            std::vector<recob::Wire> const& wires,
                            std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
                            PeakFitterMrqdt const& peak_fitter_mrqdt,
                            HitFilterAlg const& hit_filter_alg);

This signature makes the algorithm boundary obvious.

* ``cfg`` is immutable configuration.
* ``wires`` is the explicit input product.
* the helper algorithms are dependencies passed in from outside, and
* the return value is the output product.

That is the refactoring target for many *art* modules: not a callback method,
but a normal computation with explicit dependencies.

What to Move Out of the Algorithm
---------------------------------

The following concerns usually belong outside the algorithmic core.

* framework configuration retrieval,
* event product lookup,
* service lookup,
* plugin construction,
* output insertion into the event, and
* framework registration macros or declarations.

Moving these concerns out of the algorithm often reveals that the core logic is
smaller than expected.

What May Stay in the Algorithm Temporarily
------------------------------------------

A migration can proceed even if the algorithm still contains some transitional
framework assumptions, as long as those assumptions are visible and localized.

In the ``gauss_hit_finder`` example, comments explicitly mark temporary
limitations around geometry support and comparison-oriented output. That is a
reasonable intermediate state. The important point is that such concerns are not
hidden behind deep framework coupling.

Framework Boundary in the Example
---------------------------------

In ``register_find_hits_with_gaussians.cpp``, the *Phlex* side performs the
boundary work.

First, configuration is collected:

.. code-block:: cpp

   examples::find_hits_with_gaussians_cfg cfg = {
     .filter_hits = config.get<bool>("filter_hits"),
     .long_max_hits_vec = config.get<std::vector<int>>("long_max_hits_vec"),
     .long_pulse_width_vec = config.get<std::vector<int>>("long_pulse_width_vec"),
     .max_multi_hit = config.get<int>("max_multi_hit"),
     .area_method = config.get<int>("area_method"),
     .area_norms_vec = config.get<std::vector<double>>("area_norms_vec"),
     .chi2_ndf = config.get<double>("chi2_ndf"),
     .pulse_height_cuts = config.get<std::vector<float>>("pulse_height_cuts"),
     .pulse_width_cuts = config.get<std::vector<float>>("pulse_width_cuts"),
     .pulse_ratio_cuts = config.get<std::vector<float>>("pulse_ratio_cuts")
   };

Then helper objects are constructed and captured in a lambda. Finally, that
lambda is registered as the framework transformation.

Why This Split Matters
----------------------

This split matters because it creates a stable seam in the code.

Once an algorithm is separated from its framework boundary:

* it can be tested more directly,
* it can be compared against the original implementation more easily,
* it can be reused in multiple contexts, and
* the final mapping to *Phlex* becomes explicit rather than implicit.

Refactoring Checklist
---------------------

Before moving on to conceptual mapping, verify the following.

1. Can the core algorithm be described without mentioning framework callbacks?
2. Are all real inputs represented in the function signature or constructor?
3. Are all outputs represented as return values or explicit output objects?
4. Is configuration collected outside the algorithm?
5. Is framework I/O isolated near the registration boundary?
6. Are remaining framework assumptions documented as transitional issues?

Common Refactoring Mistakes
---------------------------

The following patterns usually indicate that refactoring is incomplete.

* A helper class still reaches into framework state because that was convenient
  in the original module.
* Configuration parsing remains spread across several helper objects.
* The algorithm still mutates framework-owned output containers directly.
* Service access remains hidden inside low-level logic.
* The new code preserves the old module structure even when the algorithm itself
  is much simpler.

The goal is not to preserve familiar structure. The goal is to expose the real
computation cleanly enough that it can be expressed in *Phlex*.
