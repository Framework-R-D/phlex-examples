Preparation
===========

The objective of the preparation step is to remove, reduce, or isolate *art* concepts that would otherwise leak deep into algorithm code and make later migration more complicated than necessary.
This is not yet the point where a module is rewritten in *Phlex* terms.

In many code bases, *art* concepts appear not only at the framework boundary but also deep inside algorithm code.
That is usually the result of incremental history rather than a true requirement.
The first task is therefore to identify which framework constructs are essential, which are merely convenient, and which can be confined to a narrow boundary during a staged migration.

For each *art* concept that appears in a code base:

1. determine whether the concept is essential to persisted framework behavior,
2. remove it where it is only being used for convenience,
3. isolate it at the framework boundary where it is still required, and
4. convert the algorithmic core to ordinary C++ inputs and outputs.

Preparation is successful when:

* framework-specific access patterns are pushed toward the top-level boundary,
* hidden dependencies become explicit,
* data flow is easier to see,
* ownership is easier to reason about,
* event-scoped state is localized, and
* algorithm code can start to look like ordinary C++.


The goal of preparation is not to remove every *art* type immediately.
The goal is to decide which framework concepts represent persisted behavior that must be preserved and which ones are only convenience mechanisms that should be reduced to ordinary C++ before migration.
The sections below group the main preparation work into a small number of useful categories.

``art::Ptr`` and Pointer-Like Access
------------------------------------

``art::Ptr`` combines object access with framework identity and provenance.
That is sometimes required, especially when a module writes products whose meaning depends on persisted cross-product identity.
In many code paths, however, ``art::Ptr`` is only being used as a convenient way to refer to an object that is already present in a collection.

For preparation purposes, the key distinction is whether the code really needs framework identity.
If the code only needs read-only object access, stable ordering within one collection, a simple relationship represented by an index or key, or temporary navigation during one computation, ``art::Ptr`` can usually be removed.
As a practical rule, a module typically needs ``art::Ptr`` only when it places an ``art::Ptr``-based product into the event, for example ``art::Assns``, ``art::PtrVector``, ``art::PtrMaker``-produced pointers, or a ``std::vector<art::Ptr<T>>`` written to the event.
If provenance or persisted identity is genuinely required, or if association chaining depends on ``art::Ptr<T>::key()``, then ``art::Ptr`` may need to remain at the framework boundary during an intermediate migration step.

The preparation work is therefore to inventory ``art::Ptr`` and ``fill_ptr_vector`` usage, separate persisted-identity cases from convenience usage, and replace the convenience cases with ordinary data access.
In helper code that usually means passing object values, references, indices, a ``std::vector<T> const&``, a ``std::vector<T const*> const&``, or a lookup table built at the boundary.
Any unavoidable ``art::Ptr`` usage should stay near framework I/O, and the remaining identity-sensitive cases should be recorded as explicit migration-design items.

Single-element access from a product can often be simplified to a reference:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<T>> h;
   evt.getByLabel(label, h);
   art::Ptr<T> p(h, 0);
   p->method();

   // After
   T const& p = evt.getProduct<std::vector<T>>(label).at(0);
   p.method();

Loop-local pointer construction often disappears entirely:

.. code-block:: cpp

   // Before
   for (size_t i = 0; i < h->size(); ++i) {
     art::Ptr<T> p(h, i);
     p->method();
   }

   // After
   for (size_t i = 0; i < h->size(); ++i) {
     T const& p = h->at(i);
     p.method();
   }

``fill_ptr_vector`` plus ``FindManyP`` can often become ``FindMany`` when only object access is needed:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<T>> h;
   std::vector<art::Ptr<T>> v;
   if (evt.getByLabel(label, h)) art::fill_ptr_vector(v, h);
   art::FindManyP<U> fm(v, evt, label2);
   std::vector<art::Ptr<U>> items = fm.at(i);
   items[j]->method();

   // After
   auto const h = evt.getHandle<std::vector<T>>(label);
   if (!h) return;
   art::FindMany<U> fm(h, evt, label2);
   std::vector<U const*> const& items = fm.at(i);
   items[j]->method();

When a helper only reads the object, update its signature to accept the object rather than the framework pointer:

.. code-block:: cpp

   // Before
   void f(art::Ptr<T> const& p) { p->method(); }

   // After
   void f(T const& p) { p.method(); }

When choosing navigation helpers, use ``FindMany<T>`` when only the pointed-to objects are needed and no further association chaining is required.
Keep ``FindManyP<T>`` only when the code must chain into another association and depends on ``art::Ptr<T>::key()`` as the correct identifier.
Do not substitute pointer arithmetic for ``.key()``.
When passing collections into helper code, prefer ``std::vector<T> const&`` when the original product can be passed, ``std::vector<T const*> const&`` for a filtered subset, and ``std::vector<art::Ptr<T>>`` only when identity semantics still matter.

When ``art::Ptr`` usage disappears, related includes often disappear as well, including ``canvas/Persistency/Common/Ptr.h``, ``canvas/Persistency/Common/FindManyP.h`` when ``FindMany`` is sufficient, and ``lardata/Utilities/AssociationUtil.h`` when it was only needed for ``fill_ptr_vector``.

AI tools are useful here because many ``art::Ptr`` uses are repetitive and can be classified mechanically.
They can inventory ``art::Ptr`` and ``fill_ptr_vector`` usage, classify each case as persisted-output, association-chaining, or convenience, propose local rewrites to references or raw pointers, identify stale includes, and flag helper signatures that still expose framework pointer types.
Review those changes carefully when a module writes associations or other pointer-based products, when ``FindManyP`` may still be required for chaining, when the code depends on ``.key()`` or provenance semantics, or when a helper stores pointers beyond local event scope.
:ref:`Appendix A <art-ptr-removal>` is a self-contained AI memory file covering this task: it contains a candidate-finding script, all replacement patterns, decision rules, and a completed example, and is designed to be provided directly to an AI tool as reusable context when performing this preparation work.

Data Product Retrieval and Event-Boundary Access
------------------------------------------------

*art* naturally encourages product retrieval inside module callbacks, but that convenience often hides the real inputs to the algorithm.
When helper code pulls products directly from the event, its dependencies become implicit and the code is harder to test, reuse, or map into *Phlex* dataflow declarations.

The useful preparation rule is simple: move retrieval to the top-level framework boundary and convert retrieved products into explicit function parameters.
Identify every call path that reaches into the event, list the actual products the algorithm requires, remove event access from helper classes unless they are intentionally part of the framework layer, and keep ``art::Handle`` objects local to the event callback.
``art::Handle`` objects do not outlive the event and must never be stored as module data members.
More generally, event-scoped products, pointers, references, and derived data should not be cached across events.

When retrieving data from the event, prefer ``evt.getProduct<C>(tag)`` when only the product is needed downstream, ``evt.getValidHandle<C>(tag)`` when a handle is required and the product must exist, and ``evt.getHandle<C>(tag)`` when the product may legitimately be absent.
New two-argument ``getByLabel(tag, handle)`` code should not be introduced.
If a handle is optional, test it with ``if (!h)`` rather than an explicit ``.isValid()`` check.
If older code throws an explicit exception when a handle is invalid, prefer ``getProduct`` when no handle is needed or ``getValidHandle`` when the handle is needed, and remove the manual validity check.

When a cached product pointer or reference is removed from a class, update the helper method signature to accept the collection by ``const&`` and pass it at all call sites.
AI tools can help locate retrieval APIs, group them by usage pattern, propose ``getProduct`` or ``getValidHandle`` replacements for legacy ``getByLabel`` code, identify helper methods that should accept explicit data instead of an event or handle, and detect stale ``Handle.h`` includes.
Review those changes carefully when optional products are part of the intended behavior, since that is where ``getHandle`` rather than ``getValidHandle`` may still be correct.

The two-argument ``getByLabel`` is the most common legacy retrieval pattern in *larreco* modules.
It appears in nearly every module that reads event products, including ``GausHitFinder_module.cc``, ``FFTHitFinder_module.cc``, and ``ClusterCheater_module.cc``.
The preparation step is to replace it with the appropriate modern alternative.

When only the product is needed and a handle is not required downstream, replace ``getByLabel`` with ``getProduct``:

.. code-block:: cpp

   // Before (GausHitFinder_module.cc, FFTHitFinder_module.cc)
   art::Handle<std::vector<recob::Wire>> wireVecHandle;
   evt.getByLabel(fCalDataModuleLabel, wireVecHandle);

   // After
   auto const& wires = evt.getProduct<std::vector<recob::Wire>>(fCalDataModuleLabel);

When a handle is still required, for example because ``art::Ptr`` construction from the handle is needed to populate associations, replace ``getByLabel`` with ``getValidHandle``:

.. code-block:: cpp

   // Before (GausHitFinder_module.cc, FFTHitFinder_module.cc)
   art::Handle<std::vector<recob::Wire>> wireVecHandle;
   evt.getByLabel(fCalDataModuleLabel, wireVecHandle);
   // handle used to construct art::Ptr<recob::Wire>(wireVecHandle, wireIter)

   // After
   auto const wireVecHandle = evt.getValidHandle<std::vector<recob::Wire>>(fCalDataModuleLabel);
   // art::Ptr<recob::Wire>(wireVecHandle, wireIter) still works

When the product may legitimately be absent, use ``getHandle`` and test with ``if (!h)`` rather than an explicit ``.isValid()`` call:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<recob::Hit>> hitcol;
   evt.getByLabel(fHitModuleLabel, hitcol);
   if (!hitcol.isValid()) return;

   // After
   auto const hitcol = evt.getHandle<std::vector<recob::Hit>>(fHitModuleLabel);
   if (!hitcol) return;

When ``getByLabel`` is combined with ``fill_ptr_vector``, as in ``ClusterCheater_module.cc``, replace the handle declaration and retrieval while leaving ``fill_ptr_vector`` in place if ``art::Ptr`` is still required:

.. code-block:: cpp

   // Before (ClusterCheater_module.cc)
   art::Handle<std::vector<recob::Hit>> hitcol;
   evt.getByLabel(fHitModuleLabel, hitcol);
   std::vector<art::Ptr<recob::Hit>> hits;
   art::fill_ptr_vector(hits, hitcol);

   // After
   auto const hitcol = evt.getValidHandle<std::vector<recob::Hit>>(fHitModuleLabel);
   std::vector<art::Ptr<recob::Hit>> hits;
   art::fill_ptr_vector(hits, hitcol);

When a helper class or tool receives the event object and calls retrieval APIs internally, retrieval should instead be performed at the module boundary and the product passed in explicitly.
``TrackProducerFromTrack_module.cc`` in ``larreco/TrackFinder/`` illustrates the boundary shape after this preparation: retrieval is done in ``produce()`` with ``getValidHandle`` before any helper is invoked, and helpers receive the retrieved data rather than reaching into the event themselves:

.. code-block:: cpp

   // Before: helper receives the event and retrieves data internally
   class SomeAlg {
     void run(art::Event const& evt) {
       art::Handle<std::vector<recob::Track>> tracks;
       evt.getByLabel(fLabel, tracks);
       // processes tracks internally
     }
   };
   fAlg.run(evt);

   // After: module retrieves at the boundary; helper receives explicit input
   class SomeAlg {
     void run(std::vector<recob::Track> const& tracks) {
       // processes tracks directly
     }
   };
   auto const& tracks = evt.getProduct<std::vector<recob::Track>>(fLabel);
   fAlg.run(tracks);

Associations and Navigation
--------------------------------

Associations encode relationships among products, but in existing code they are often also used as a convenient navigation mechanism.
Preparation is the point where those two roles need to be separated.
If they are not, framework-era navigation patterns leak into code that should instead operate on explicit relationships in ordinary C++ data.

The main preparation task is to identify associations used only to traverse from one product to another and decide whether the relationship is truly part of the domain model or only a retrieval pattern.
For domain relationships, define an explicit representation that the algorithm can understand directly.
For retrieval-only usage, keep the association handling at the framework boundary and pass simpler structures inward.
Association-aware logic should be preserved only where persisted identity is part of the intended behavior.

Associations may still remain temporarily at the framework boundary during a staged migration, especially when current module output is still defined in terms of ``art::Assns``.
Even then, the preparation goal is to keep that representation out of reusable algorithm code wherever possible.
AI tools can help inventory helpers such as ``FindOneP``, ``FindManyP``, and ``art::Assns`` construction and distinguish navigation-only use from persisted-output use.
Review those changes carefully when the code builds new associations or when helper logic depends on stable framework identity rather than simple object access.

Services, Plugins, and Other Hidden Dependencies
------------------------------------------------

Service access and framework-managed plugins both make it easy to obtain shared objects or runtime-selected behavior, but they also tend to hide what the code actually depends on.
A service may really be acting as configuration, geometry access, a utility object, a cache, or a true framework-owned service.
Likewise, helper logic may be packaged as a plugin because of project history rather than because runtime extensibility is still needed.

The useful preparation step is to inventory those dependencies and classify what the code is really obtaining.
Convert non-framework concerns into explicit parameters, constructor arguments, or ordinary helper objects where possible.
Identify which plugin boundaries are truly required for runtime extensibility, convert purely local helper logic into ordinary classes or functions, move configuration parsing out of helper logic where practical, and leave only genuinely framework-owned interactions at the boundary.
Any remaining service or plugin dependencies that block full migration should be documented explicitly.

AI tools can trace service calls across helper classes, suggest constructor or function parameters that make those dependencies explicit, and identify helpers that are framework-discovered only because of historical structure.
Review those changes carefully when a service imposes ordering, thread-safety, or lifecycle constraints that are not obvious from the immediate call site, or when a plugin boundary is part of the package's public extension mechanism and therefore needs to remain stable during migration.

*art* Services
~~~~~~~~~~~~~~

An *art* service is a globally accessible stateful object with a specific lifecycle: it is constructed before the first module and destroyed after the last.
Services can register callbacks for framework transitions that are not accessible to modules, depend on other services through ``art::ServiceHandle``, and be polymorphic.
That broad capability makes them convenient, but also makes them a frequent source of hidden dependencies that complicate migration.

Before treating a service dependency as unavoidable, it is worth asking what the code is actually obtaining from the service and whether a simpler alternative suffices.

Alternatives to services
^^^^^^^^^^^^^^^^^^^^^^^^^

Many common reasons for reaching for a service do not require one.
The following patterns are expected to be provided without a service mechanism in *Phlex*:

* **Message logging.** Standard logging libraries (e.g., ``spdlog``, ``std::cerr``) are ordinary C++ and need no framework wrapper.
* **Profiling and monitoring.** Facilities such as ``TimeTracker`` and ``MemoryTracker`` are infrastructure concerns that can be provided by the framework runtime without exposing a service handle to user code.
* **Global-state wrappers.** Objects like ``TFileService`` exist to manage global state in external libraries (e.g., ROOT).
  That global state is a real need, but the management object does not need to be obtained through ``art::ServiceHandle``; it can be provided as a constructor argument or explicit parameter.
* **Singleton-like shared objects (e.g., Geometry).** Objects that are read-only after initialization and shared across algorithms are not fundamentally services.
  In *Phlex*, such objects are data products belonging to a long-lived data family (e.g., a job-level or run-level family) and are provided to algorithms through the normal data-flow mechanism, not through a globally accessible handle.
* **Conditions-style and database-derived data.** Calibration offsets, channel maps, and other data that vary by run or time interval but are independent of individual events are exactly what framework-managed data families are for.
  Rather than wrapping a database client in a service and calling it from algorithm code, the data should be fetched by a dedicated algorithm and placed into the appropriate data family so that downstream algorithms receive it as an explicit input.

When one of these patterns is in use, the preparation task is to remove the ``art::ServiceHandle`` call from algorithm code entirely and replace it with an explicit parameter, constructor argument, or ordinary helper object.

Cases where a service dependency may remain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Some service uses are harder to eliminate before a full migration and may remain at the framework boundary during a staged preparation:

* The service genuinely needs the *art* lifecycle — for example, it registers callbacks for run, subrun, or job transitions that are not accessible to a plain module.
* The service is part of a stable public extension interface that other packages depend on and that cannot be changed independently.
* The service enforces singleton or ordering constraints that are a real framework requirement, not merely a convenience.

Even in these cases the preparation goal is the same: keep the service call confined to the module boundary and prevent it from reaching into algorithm code.
The algorithm should receive whatever data or object the service provides as an ordinary C++ argument, not by constructing a ``ServiceHandle`` internally.

Preparing the boundary: separating framework and algorithm code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most common problematic pattern is an algorithm that constructs a ``ServiceHandle`` inside its own body:

.. code-block:: cpp

   // Before: service access embedded inside algorithm — problematic
   Tracks make_tracks(Hits const& hits)
   {
     art::ServiceHandle<Calibration> calibration;
     ScalarOffset const& offset = calibration->Offset();
     // ...
   }

   void TrackMaker::produce(art::Event& e)
   {
     Hits const& hits = e.getProduct<Hits>("GoodHits");
     Tracks tracks = make_tracks(hits);
     e.put(std::make_unique<Tracks>(std::move(tracks)), "GoodTracks");
   }

This couples the algorithm directly to the framework, hides the calibration data as an implicit dependency, makes provenance incomplete, and makes thread-safety unclear because ``ServiceHandle`` access patterns vary by service.

The preparation fix is to retrieve the service data at the module boundary and pass it to the algorithm as an explicit argument:

.. code-block:: cpp

   // After: service access confined to module boundary
   Tracks make_tracks(Hits const& hits, ScalarOffset const& offset)
   {
     // no framework types; offset is an ordinary C++ value
     // ...
   }

   void TrackMaker::produce(art::Event& e)
   {
     Hits const& hits = e.getProduct<Hits>("GoodHits");
     art::ServiceHandle<Calibration> calibration;
     Tracks tracks = make_tracks(hits, calibration->Offset());
     e.put(std::make_unique<Tracks>(std::move(tracks)), "GoodTracks");
   }

After this change the algorithm is framework-independent, its dependency on the calibration offset is explicit, and thread-safety is straightforward to reason about because the algorithm itself holds no framework state.

This is the intended preparation shape even when the *art* service cannot yet be removed.
Once the codebase uses *Phlex*, the same algorithm function requires no change at all; the framework registration simply declares where the offset comes from:

.. code-block:: cpp

   // Phlex target: algorithm unchanged; wiring declared separately
   Tracks make_tracks(Hits const& hits, ScalarOffset const& offset)
   { /* unchanged */ }

   REGISTER(m)
   {
     m.with(make_tracks)
      .transform("GoodHits"_in("event"),
                 "CalibOffset"_in("calibration"))
      .to("GoodTracks")
      .for_each("event");
   }

The algorithm is the same function in both the prepared *art* code and the final *Phlex* registration.
That continuity is the practical payoff of isolating service access at the module boundary during preparation.

When completing this preparation work, check that:

* ``art::ServiceHandle`` does not appear inside any function or class that is not itself a module or framework-boundary object,
* every value extracted from a service is passed as an explicit parameter or constructor argument to downstream code,
* helpers that previously received an event or service handle now accept the extracted value directly, and
* any service dependencies that genuinely cannot be removed are recorded as explicit migration-design items.

Mutable Module State
---------------------

Long-lived module state often grows around the event-loop style of *art* modules.
Over time, that state may mix configuration, scratch space, monitoring, counters, cached products, and per-event data.
A common migration barrier is state whose lifetime is broader than the computation that actually needs it, because that makes the true inputs and outputs of the algorithm harder to see.

Preparation starts by classifying state as immutable configuration, per-event input, temporary scratch storage, accumulated reporting or monitoring, or a true long-lived cache.
Immutable configuration should move into a plain configuration object.
Per-event data should stay out of long-lived module members.
Scratch storage should be localized to the computation that needs it.
Any remaining state that affects thread safety, determinism, or lifecycle should be documented explicitly.

AI tools can help scan module members and helper classes for cached handles, per-event products, and mutable state that should become local variables or explicit parameters.
Review those changes carefully when long-lived caches are intentional, since those cases may encode performance or lifecycle assumptions that are not visible from a single source file.

Cross-Cutting Rules
-------------------

A few preparation rules apply across all *art* concepts:

* keep framework retrieval and framework-owned identity at the module boundary,
* do not cache event-scoped objects across events,
* prefer explicit inputs over implicit framework access,
* prefer ordinary C++ containers and references in helper code, and
* preserve *art*-specific types only where persisted behavior requires them.

Preparation Checklist
---------------------

Before moving to refactoring, review the code with the following questions.

1. Which *art* concepts still appear in the code, and why?
2. Which remaining uses are required for persisted outputs or framework-owned identity?
3. Which uses have been reduced to ordinary C++ data at the algorithm boundary?
4. Which helper interfaces still expose framework types unnecessarily?
5. Which remaining dependencies need a deliberate migration design rather than
   a mechanical cleanup?

Preparation Example: Explicit Inputs
------------------------------------

The ``gauss_hit_finder`` example in this repository shows the target shape for a prepared algorithm boundary:

.. code-block:: cpp

   std::vector<recob::Hit>
   find_hits_with_gaussians(find_hits_with_gaussians_cfg const& cfg,
                            std::vector<recob::Wire> const& wires,
                            std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
                            PeakFitterMrqdt const& peak_fitter_mrqdt,
                            HitFilterAlg const& hit_filter_alg);

This boundary is migration-friendly because:

* configuration is explicit,
* the input product is explicit,
* helper algorithms are explicit dependencies, and
* the result is returned directly.

That is the intended result of preparation work, even before the final *Phlex* mapping is written.
