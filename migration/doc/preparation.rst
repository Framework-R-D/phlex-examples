Framework and Domain Logic Separation
=====================================

The objective of this step is to remove, reduce, or isolate *art* concepts
that would otherwise leak deep into algorithm code and make later migration
more complicated than necessary. The goal is not to remove every *art* type
immediately, but to decide which framework concepts are required to preserve
the module's framework-visible output and which are only convenience
mechanisms that should be reduced to ordinary C++ before migration.

In many code bases, *art* concepts appear throughout the module code as a
result of incremental history rather than a true requirement. The first task is
to identify which framework constructs are essential and which are merely
convenient.

For each *art* concept that appears in a code base:

1. determine whether the concept is required to preserve the module's
   framework-visible output and identity, or whether it is only an
   implementation convenience,
2. remove it where it is only being used for convenience,
3. isolate it in framework-boundary code where it is still required, and
4. convert the algorithm code to ordinary C++ inputs and outputs.

Separation is successful when:

* framework-specific access patterns are pushed toward the top-level boundary,
* hidden dependencies become explicit,
* data flow is easier to see,
* ownership is easier to reason about,
* event-scoped state is localized, and
* algorithm code starts to look like ordinary C++.

Cross-Cutting Rules
-------------------

A few separation rules apply across all *art* concepts:

* keep framework retrieval and framework-owned identity at the module boundary,
* do not cache event-scoped objects across events,
* prefer explicit inputs over implicit framework access,
* prefer ordinary C++ containers and references in helper code, and
* preserve *art*-specific types only where persisted behavior requires them.

``art::Ptr`` and Pointer-Like Access
------------------------------------

``art::Ptr`` combines object access with framework identity and provenance.
That is sometimes required, especially when a module writes products whose
meaning depends on persisted cross-product identity. In many code paths,
however, ``art::Ptr`` is only being used as a convenient way to refer to an
object that is already present in a collection.

The key distinction is whether the code really needs framework identity.
``art::Ptr`` can usually be removed when the code only needs:

* read-only object access,
* stable ordering within one collection,
* a simple relationship represented by an index or key, or
* temporary navigation during one computation.

As a practical rule, a module typically needs ``art::Ptr`` only when it places
an ``art::Ptr``-based product into the event, for example ``art::Assns``,
``art::PtrVector``, ``art::PtrMaker``-produced pointers, or a
``std::vector<art::Ptr<T>>`` written to the event. If provenance or persisted
identity is genuinely required, or if association chaining depends on
``art::Ptr<T>::key()``, then ``art::Ptr`` may need to remain at the framework
boundary during an intermediate migration step.

The separation work is to inventory ``art::Ptr`` and ``fill_ptr_vector``
usage, separate persisted-identity cases from convenience usage, and replace
the convenience cases with ordinary data access. In helper code that usually
means passing object values, references, indices, a ``std::vector<T> const&``,
a ``std::vector<T const*> const&``, or a lookup table built at the boundary.
Any unavoidable ``art::Ptr`` usage should stay near framework I/O.

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
   for (auto const& p : *h) {
     p.method();
   }

``FindManyP`` can often become ``FindMany`` when only object access is needed:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<T>> h;
   evt.getByLabel(label, h);
   std::vector<art::Ptr<T>> v;
   art::fill_ptr_vector(v, h);
   art::FindManyP<U> fm(v, evt, label2);
   std::vector<art::Ptr<U>> const& items = fm.at(i);
   items[j]->method();

   // After
   auto const h = evt.getValidHandle<std::vector<T>>(label);
   art::FindMany<U> fm(h, evt, label2);
   std::vector<U const*> const& items = fm.at(i);
   items[j]->method();

When a helper only reads the object, update its signature to accept the object
rather than the framework pointer:

.. code-block:: cpp

   // Before
   void f(art::Ptr<T> const& p) { p->method(); }

   // After
   void f(T const& p) { p.method(); }

Use ``FindMany<T>`` when only the pointed-to objects are needed and no further
association chaining is required. Keep ``FindManyP<T>`` only when the code must
chain into another association and depends on ``art::Ptr<T>::key()`` as the
identifier. Do not substitute pointer arithmetic for ``.key()``.

When passing collections into helper code, prefer ``std::vector<T> const&``
when the original product can be passed, ``std::vector<T const*> const&`` for
a filtered subset, and ``std::vector<art::Ptr<T>>`` only when identity
semantics still matter.

When ``art::Ptr`` usage disappears, related includes often disappear as well,
including ``canvas/Persistency/Common/Ptr.h``,
``canvas/Persistency/Common/FindManyP.h`` when ``FindMany`` is sufficient, and
``lardata/Utilities/AssociationUtil.h`` when it was only needed for
``fill_ptr_vector``.

AI tools are useful here because many ``art::Ptr`` uses are repetitive and can
be classified mechanically. They can inventory usage, classify each case as
persisted-output, association-chaining, or convenience, propose rewrites to
references or raw pointers, identify stale includes, and flag helper signatures
that still expose framework pointer types. Review those changes carefully when
a module writes associations or other pointer-based products, when
``FindManyP`` may still be required for chaining, when the code depends on
``.key()`` or provenance semantics, or when a helper stores pointers beyond
local event scope.

:ref:`Appendix A <art-ptr-removal>` is a self-contained AI memory file
covering this task: it contains a candidate-finding script, all replacement
patterns, decision rules, and a completed example, and is designed to be
provided directly to an AI tool as reusable context when performing this
separation work.

Data Product Retrieval and Event-Boundary Access
------------------------------------------------

*art* naturally encourages product retrieval inside module callbacks, but that
convenience often hides the real inputs to the algorithm. When helper code
pulls products directly from the event, its dependencies become implicit and
the code becomes harder to test, reuse, or map into *Phlex* dataflow
declarations.

The separation rule is simple: move retrieval to the top-level framework
boundary and pass retrieved products as explicit function parameters. Identify
every call path that reaches into the event, list the actual products the
algorithm requires, and remove event access from helper classes unless they are
intentionally part of the framework layer.

``art::Handle`` objects do not outlive the event and must never be stored as
module data members. More generally, event-scoped products, pointers,
references, and derived data must not be cached across events.

When retrieving data from the event, choose the appropriate call:

* ``evt.getProduct<C>(tag)`` when only the product is needed downstream,
* ``evt.getValidHandle<C>(tag)`` when a handle is required and the product
  must exist,
* ``evt.getHandle<C>(tag)`` when the product may legitimately be absent.

Do not introduce new ``getByLabel(tag, handle)`` code. If a handle is
optional, test it with ``if (!h)`` rather than an explicit ``.isValid()``
check. If older code throws an explicit exception when a handle is invalid,
replace it with ``getProduct`` or ``getValidHandle`` and remove the manual
check.

``getByLabel`` is the most common legacy retrieval pattern in *larreco*
modules. The separation step is to replace it with the appropriate modern
alternative.

When only the product is needed:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<recob::Wire>> wireVecHandle;
   evt.getByLabel(fCalDataModuleLabel, wireVecHandle);

   // After
   auto const& wires = evt.getProduct<std::vector<recob::Wire>>(fCalDataModuleLabel);

When a handle is still required (e.g. to construct ``art::Ptr`` for
associations):

.. code-block:: cpp

   // Before
   art::Handle<std::vector<recob::Wire>> wireVecHandle;
   evt.getByLabel(fCalDataModuleLabel, wireVecHandle);

   // After
   auto const wireVecHandle =
     evt.getValidHandle<std::vector<recob::Wire>>(fCalDataModuleLabel);
   // art::Ptr<recob::Wire>(wireVecHandle, i) still works

When the product may legitimately be absent:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<recob::Hit>> hitcol;
   evt.getByLabel(fHitModuleLabel, hitcol);
   if (!hitcol.isValid()) return;

   // After
   auto const hitcol = evt.getHandle<std::vector<recob::Hit>>(fHitModuleLabel);
   if (!hitcol) return;

When ``getByLabel`` is combined with ``fill_ptr_vector`` and ``art::Ptr`` is
still required, replace only the retrieval:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<recob::Hit>> hitcol;
   evt.getByLabel(fHitModuleLabel, hitcol);
   std::vector<art::Ptr<recob::Hit>> hits;
   art::fill_ptr_vector(hits, hitcol);

   // After
   auto const hitcol = evt.getValidHandle<std::vector<recob::Hit>>(fHitModuleLabel);
   std::vector<art::Ptr<recob::Hit>> hits;
   art::fill_ptr_vector(hits, hitcol);

When a helper class receives the event object and calls retrieval APIs
internally, retrieval should instead be performed at the module boundary and
the product passed in explicitly:

.. code-block:: cpp

   // Before: helper retrieves data from the event
   class SomeAlg {
     void run(art::Event const& evt) {
       auto const& tracks = evt.getProduct<std::vector<recob::Track>>(fLabel);
       // ...
     }
   };
   fAlg.run(evt);

   // After: module retrieves at the boundary; helper receives explicit input
   class SomeAlg {
     void run(std::vector<recob::Track> const& tracks) {
       // ...
     }
   };
   auto const& tracks = evt.getProduct<std::vector<recob::Track>>(fLabel);
   fAlg.run(tracks);

AI tools can help locate retrieval APIs, group them by usage pattern, propose
``getProduct`` or ``getValidHandle`` replacements for legacy ``getByLabel``
code, identify helper methods that should accept explicit data instead of an
event or handle, and detect stale ``Handle.h`` includes. Review those changes
carefully when optional products are part of the intended behavior.

Associations and Navigation
---------------------------

In the current *Phlex* design, associations are still under development.

Associations may remain temporarily at the framework boundary during a staged
migration, especially when module output is still defined in terms of
``art::Assns``. Even then, keep that representation out of reusable algorithm
code wherever possible.

AI tools can help inventory helpers such as ``FindOneP``, ``FindManyP``, and
``art::Assns`` construction and distinguish navigation-only use from
persisted-output use. Review those changes carefully when the code builds new
associations or when helper logic depends on stable framework identity rather
than simple object access.

Services
--------

*Phlex* does not have a concept of services.

An *art* service is a globally accessible stateful object: constructed before
the first module, destroyed after the last, able to register callbacks for
framework transitions, and able to depend on other services through
``art::ServiceHandle``. That flexibility is also a frequent source of hidden
dependencies that complicate migration.

The key rule is: ``art::ServiceHandle`` belongs at the framework boundary, not
inside reusable algorithm code.

Preparing existing code
^^^^^^^^^^^^^^^^^^^^^^^

Start by inventorying every place that constructs a ``ServiceHandle`` or
depends on a service-provided object. For each use, identify what the
downstream code actually needs:

* a read-only value or data structure,
* a helper object that can be passed in explicitly,
* a side effect such as logging or output production, or
* a true framework-lifecycle feature that cannot yet be represented another
  way.

In most cases, the module should retrieve the service-provided data at the
boundary and pass ordinary C++ inputs into the algorithm. Concretely:

* remove ``art::ServiceHandle`` construction from algorithms, helpers, and
  utility classes,
* retrieve the needed value or object in the module callback,
* replace hidden service access with explicit function parameters or
  constructor arguments,
* treat read-only shared state (geometry, calibration constants, channel maps)
  as future long-lived data inputs rather than global handles, and
* record any remaining lifecycle-driven service dependencies as explicit
  migration-design items.

The most common problematic pattern is an algorithm that constructs a
``ServiceHandle`` inside its own body:

.. code-block:: cpp

   // Before: service access inside algorithm — problematic
   Tracks make_tracks(Hits const& hits)
   {
     art::ServiceHandle<Calibration> calibration;
     ScalarOffset const& offset = calibration->Offset();
     // ...
   }

   void TrackMaker::produce(art::Event& e)
   {
     auto const& hits = e.getProduct<Hits>("GoodHits");
     Tracks tracks = make_tracks(hits);
     e.put(std::make_unique<Tracks>(std::move(tracks)), "GoodTracks");
   }

This couples the algorithm directly to the framework, hides the calibration
data as an implicit dependency, and makes provenance incomplete.

The fix is to retrieve the service data at the module boundary and pass it as
an explicit argument:

.. code-block:: cpp

   // After: service access confined to module boundary
   Tracks make_tracks(Hits const& hits, ScalarOffset const& offset)
   {
     // no framework types; offset is an ordinary C++ value
     // ...
   }

   void TrackMaker::produce(art::Event& e)
   {
     auto const& hits = e.getProduct<Hits>("GoodHits");
     art::ServiceHandle<Calibration> calibration;
     Tracks tracks = make_tracks(hits, calibration->Offset());
     e.put(std::make_unique<Tracks>(std::move(tracks)), "GoodTracks");
   }

After this change the algorithm is framework-independent, the dependency on
the calibration offset is explicit, and the algorithm itself holds no framework
state.

This is the intended separation shape even when the *art* service cannot yet
be removed. Once the codebase uses *Phlex*, the same algorithm function can
remain unchanged. The framework registration declares where the offset comes
from, but the algorithm code is not touched.

When completing this work, check that:

* ``art::ServiceHandle`` does not appear inside any function or class that is
  not itself a module or framework-boundary object,
* every value extracted from a service is passed as an explicit parameter or
  constructor argument to downstream code,
* helpers that previously received an event or service handle now accept the
  extracted value directly, and
* any service dependencies that genuinely cannot be removed are recorded as
  explicit migration-design items.

When to use a service
^^^^^^^^^^^^^^^^^^^^^

The migration guidance above is for code that already uses services. If you
are considering introducing a new service, first ask whether the capability
really needs to be modeled as one.

Many common uses do not require a service:

* **Message logging.** Standard logging libraries (e.g., ``spdlog``,
  ``std::cerr``) are ordinary C++ and need no framework wrapper.
* **Profiling and monitoring.** Facilities such as ``TimeTracker`` and
  ``MemoryTracker`` are infrastructure concerns provided by the framework
  runtime; they do not need to expose a service handle to user code.
* **Global-state wrappers.** Objects like ``TFileService`` manage global state
  in external libraries. That is a real need, but the management object can be
  provided as a constructor argument or explicit parameter rather than through
  ``art::ServiceHandle``.
* **Shared read-only objects (e.g., Geometry).** Objects that are read-only
  after initialization are not fundamentally services. In *Phlex*, such objects
  are data products belonging to a long-lived data family and are provided to
  algorithms through the normal dataflow, not through a global handle.
* **Conditions and database-derived data.** Calibration offsets, channel maps,
  and similar data that vary by run or time interval are exactly what
  framework-managed data families are for. Rather than wrapping a database
  client in a service, the data should be fetched by a dedicated algorithm and
  placed into the appropriate data family so downstream algorithms receive it
  as an explicit input.

Some service uses are harder to eliminate before a full migration and may
remain at the framework boundary during a staged separation:

* The service genuinely needs the *art* lifecycle (e.g. it registers callbacks
  for run, subrun, or job transitions not accessible to a plain module).
* The service is part of a stable public interface that other packages depend
  on and cannot be changed independently.
* The service enforces singleton or ordering constraints that are a real
  framework requirement, not merely a convenience.

Even then, the separation goal is the same: keep the service call confined to
the module boundary and pass whatever the service provides as an ordinary C++
argument to the algorithm.
