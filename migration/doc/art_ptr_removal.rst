.. _art-ptr-removal:

Appendix A: AI Memory File — Removing ``art::Ptr`` from LArSoft Modules
========================================================================

.. note::

   This appendix is an AI memory file. Copy its full contents and provide them
   to an AI assistant as context before asking it to remove ``art::Ptr`` usage
   from a module. The rules, patterns, and script below are self-contained and
   do not require the AI to consult other documentation. The assistant can then
   apply them mechanically and repeatedly across any number of module files.

Goal
----

Identify ``*_module.cc`` files that use ``art::Ptr``/``fill_ptr_vector`` but do
not need to — that is, they never place an ``art::Ptr``-based product into the
event.

A module needs ``art::Ptr`` if it puts into the event:

* ``art::Assns``
* ``art::PtrVector``
* ``art::PtrMaker``-produced pointers
* ``std::vector<art::Ptr<T>>``

Finding Candidates
------------------

The following Python script walks a source tree and identifies modules that use
``art::Ptr`` or ``fill_ptr_vector`` but produce no pointer-based output products:

.. code-block:: python

   import os, re
   srcs = '/path/to/srcs'
   no_ptr_output = []
   for root, dirs, files in os.walk(srcs, followlinks=True):  # followlinks needed for symlinked repos
       for fname in files:
           if not fname.endswith('_module.cc'): continue
           fpath = os.path.join(root, fname)
           with open(fpath) as fh: content = fh.read()
           if 'art::Ptr<' not in content and 'fill_ptr_vector' not in content: continue
           has_output = (re.search(r'\bart::Assns\b', content) or
                         re.search(r'\bart::PtrVector\b', content) or
                         re.search(r'\bart::PtrMaker\b', content) or
                         (re.search(r'vector\s*<\s*art::Ptr\s*<', content) and re.search(r'\.\s*put\s*\(', content)))
           if not has_output:
               no_ptr_output.append(fpath[len(srcs)+1:])

Replacement Patterns
--------------------

Single-element access from a handle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When ``art::Ptr`` is used only to access one element from a handle:

.. code-block:: cpp

   // Before
   art::Handle<std::vector<T>> h; evt.getByLabel(label, h);
   art::Ptr<T> p(h, 0);
   p->method();

   // After
   T const& p = evt.getProduct<std::vector<T>>(label).at(0);
   p.method();

Loop-local pointer construction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When ``art::Ptr`` is used only within a loop body and not stored or returned:

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
   // Or use range-based for with getProduct:
   for (T const& p : evt.getProduct<std::vector<T>>(label)) { p.method(); }

``fill_ptr_vector`` and ``FindManyP`` to ``getHandle`` and ``FindMany``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When ``fill_ptr_vector`` feeds ``FindManyP`` and only object access is needed:

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
   items[j]->method();  // syntax unchanged

``fill_ptr_vector`` for passing hits to ``BackTrackerService``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When ``fill_ptr_vector`` is used only to construct a pointer vector for
``BackTrackerService``:

.. code-block:: cpp

   // Before
   std::vector<art::Ptr<recob::Hit>> allhits;
   art::fill_ptr_vector(allhits, hithdl);

   // After — build pointer vector directly; avoids art::Ptr overhead
   auto const hithdl = e.getValidHandle<std::vector<recob::Hit>>(label);
   std::vector<recob::Hit const*> allhits;
   allhits.reserve(hithdl->size());
   for (auto const& hit : *hithdl) allhits.push_back(&hit);

``BackTrackerService`` now has ``std::vector<recob::Hit const*>`` overloads for
``GetSetOfTrackIds``, ``HitCollectionPurity``, and ``HitCollectionEfficiency``.

``FindOneP`` to ``FindOne``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``FindOneP<T>::at(i)`` returns ``art::Ptr<T>`` (use ``->``)
* ``FindOne<T>::at(i)`` returns ``cet::maybe_ref<T const>`` (use ``.ref().``)
* Header: ``canvas/Persistency/Common/FindOne.h``

Method signatures taking ``art::Ptr`` by const reference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Before
   void f(art::Ptr<T> const& p) { p->method(); }

   // After
   void f(T const& p)            { p.method();  }

Collection parameter preference order
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When passing a collection of data products to a helper function, prefer in this
order:

1. ``std::vector<T> const&`` — pass the product vector directly (e.g. ``*handle``);
   cleanest, no indirection.
2. ``std::vector<T const*> const&`` — raw pointer vector; use when a direct
   reference is not feasible (e.g. a filtered subset).
3. ``std::vector<art::Ptr<T>>`` — avoid; only justified when ``.key()`` is needed
   for association chaining inside the helper.

Handle acquisition preference order
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. ``evt.getProduct<C>(tag)`` — first choice whenever the handle is not needed
   downstream; returns ``C const&`` directly; throws ``art::Exception``
   automatically if missing; also remove
   ``#include "art/Framework/Principal/Handle.h"`` if no ``art::Handle`` type
   remains.
2. ``evt.getValidHandle<C>(tag)`` — use when a handle object is required (e.g. to
   construct ``FindMany``/``FindManyP``); product must always be present; no
   explicit validity check needed.
3. ``evt.getHandle<C>(tag)`` — use only when the product may legitimately be
   absent; test with ``if (!h)`` (implicit bool), never ``.isValid()``.
4. ``evt.getByLabel(tag, h)`` — obsolete two-argument form; always replace with
   one of the above.

Additional rules:

* ``art::Handle`` objects must never be data members of a module class — handles
  do not outlive the event.
* Data products (pointers, references, IDs) must never be cached as class data
  members across events — obtain them locally in ``analyze()``/``produce()``/etc.
* If existing code throws an explicit exception when a handle is invalid, replace
  with ``getProduct`` (no handle needed) or ``getValidHandle`` (handle needed),
  eliminating the manual validity check.
* When a cached data product pointer or reference is removed from a class, update
  helper method signatures to accept the collection by ``const&`` and pass it at
  every call site.

``FindMany`` versus ``FindManyP``
---------------------------------

* Use ``FindMany<T>`` when only the pointed-to objects are needed (no chaining) —
  returns ``T const*``.
* Use ``FindManyP<T>`` when association chaining is required —
  ``art::Ptr<T>::key()`` is the correct, safe way to index into the next
  association query.
* Do not use pointer arithmetic (``ptr - handle->data()``) as a substitute for
  ``.key()`` — it is fragile and error-prone.
* A module may legitimately include ``FindManyP.h`` alongside ``FindMany.h`` when
  it chains associations, even if it does not output ``art::Ptr``-based products.

Includes to Remove
------------------

When no ``art::Ptr`` remains:

* ``canvas/Persistency/Common/Ptr.h``
* ``canvas/Persistency/Common/FindManyP.h`` — replace with ``FindMany.h`` only
  when there is no association chaining; keep (or restore) when ``.key()`` is
  needed for chaining.
* ``lardata/Utilities/AssociationUtil.h`` (if only used for ``fill_ptr_vector``)

When no ``art::Handle`` remains:

* ``art/Framework/Principal/Handle.h``

Example: ``RecoCheckAna_module.cc``
------------------------------------

The ``RecoCheckAna_module.cc`` file in ``larsim/larsim/MCCheater/`` is a
completed example of this separation work:

* Removed ``FindManyP.h`` and ``Ptr.h``; replaced with ``FindMany.h``.
* All ``std::vector<art::Ptr<recob::Hit>>`` replaced with
  ``std::vector<recob::Hit const*>``.
* ``fill_ptr_vector`` replaced by direct pointer construction from
  ``getValidHandle``.
* All ``FindManyP`` replaced with ``FindMany`` throughout helper methods.
* ``BackTrackerService`` calls updated to non-Ptr overloads.
* Modern C++ throughout: structured bindings, range-for, if-with-initializer.
