Algorithm Extraction
====================

After framework and domain logic separation, the next step is algorithm
extraction. The purpose is to refactor the code so the algorithm is defined in
plain functions or small classes that contain no framework code and can be
tested independently.

This means separating the code into two parts:

Algorithm part
  Contains the domain logic. It should operate on plain C++ values,
  experiment-specific data structures, and explicitly provided helper objects,
  without framework callbacks, event objects, handles, services, or
  registration machinery.

Framework-dependent part
  Contains configuration lookup, input retrieval, helper construction,
  registration, and output publication.

The algorithm part should answer the question "what computation is being
performed?" The framework part should answer "how is this computation wired
into the framework?" A good extraction leaves the algorithm usable in an
ordinary unit test or standalone driver without bringing in framework code.

This is the key structural change that makes migration to *Phlex* practical.
Without it, migration remains a translation of framework-specific code. With
it, migration becomes a matter of binding clear inputs and outputs to a
well-defined computation. The extracted algorithm can then be tested and
compared against the original implementation.

Because many original modules have limited test coverage, extraction should
minimize changes to the core logic while moving framework-specific code into a
separate boundary layer.

A successful extraction produces code with the following properties:

* The algorithm is defined in plain functions or small classes.
* The algorithm can be called from ordinary C++.
* The algorithm does not depend on module callbacks such as ``produce()`` or
  ``analyze()``.
* The algorithm contains no direct framework code.
* Configuration is gathered once and passed explicitly.
* Input products are passed in as parameters.
* Output products are returned as values.
* Remaining framework assumptions are visible and localized.

Recommended Extraction Pattern
-------------------------------

1. Define a plain configuration structure.
2. Extract the core computation into a free function or small class.
3. Pass all required inputs explicitly.
4. Return outputs directly.
5. Keep framework registration in a separate translation unit.

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

This structure was motivated by the configuration parameters used in the existing ``GausHitFinder_module.cc`` *art* module in ``larreco``. Note that it is independent
of either *art* or *Phlex*---it is a normal C++ representation of the configuration the
algorithm needs.

The algorithm is exposed through a normal function:

.. code-block:: cpp

   using Hits = std::vector<recob::Hit>;
   using CandHitStandardPtr = std::shared_ptr<CandHitStandard>;

   Hits find_hits_with_gaussians(find_hits_with_gaussians_cfg const& cfg,
                                 std::vector<recob::Wire> const& wires,
                                 std::vector<CandHitStandardPtr> const& cand_hit_standard,
                                 PeakFitterMrqdt const& peak_fitter_mrqdt,
                                 HitFilterAlg const& hit_filter_alg);

This signature makes the algorithm boundary explicit:

* ``cfg`` is immutable configuration,
* ``wires`` is the input product,
* the helper algorithms are dependencies passed in from outside, and
* the return value is the output product.

That is the extraction target for many *art* modules: not a callback method,
but a normal computation with explicit dependencies.

The following concerns belong outside the algorithm:

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
reasonable intermediate state. The important point is that such concerns are
not hidden behind deep framework coupling.

Algorithm Extraction Checklist
-------------------------------

Before moving on to *Phlex* binding, verify the following.

1. Can the core algorithm be described without mentioning framework callbacks?
2. Are all real inputs represented in the function signature or constructor?
3. Are all outputs represented as return values or explicit output objects?
4. Is configuration collected outside the algorithm?
5. Are remaining framework assumptions documented as transitional issues?

Common Extraction Mistakes
--------------------------

The following patterns usually indicate that extraction is incomplete.

* A helper class still reaches into framework state because that was
  convenient in the original module.
* Configuration parsing remains spread across several helper objects.
* Service access remains hidden inside low-level logic.
* The new code preserves the old module structure even when the algorithm
  itself is much simpler.

The goal is not to preserve familiar structure. It is to expose the real
computation cleanly enough that it can be expressed in *Phlex*.
