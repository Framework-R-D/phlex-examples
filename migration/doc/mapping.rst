Phlex Binding
=============

Once a component has gone through framework and domain logic separation and algorithm extraction, the remaining task is to
express it in *Phlex* concepts. This is the binding stage.

The goal is not to reproduce an *art* module line by line. The goal is to
describe the same computation in terms of explicit dataflow, explicit
dependencies, explicit layering, and explicit outputs.

Binding Mindset
---------------

A useful way to think about binding is to change the guiding question.

In *art*, the question is often:

* what does this module do when the framework calls ``produce()``,
  ``beginSubRun()``, ``endSubRun()``, or ``analyze()``?

In *Phlex*, the question is instead:

* which higher-order function best describes this computation?
* what are the input products?
* what layer does the computation belong to?
* does it create, transform, accumulate, expand, or simply observe data?

That shift in viewpoint is what makes an extracted component bind naturally into
*Phlex*.

Binding to Higher-Order Functions
---------------------------------

Current *Phlex* code is organized around a small set of higher-order
registration functions. Most migrated code binds to one of the following.

``provide``
^^^^^^^^^^^

Use ``provide`` when the callable creates a product without consuming another
*Phlex* product as input.

Typical uses are:

* creating seed data for tests,
* exposing configuration-derived or job-level data as products, and
* publishing data that originates at the framework boundary.

A current registration shape looks like this:

.. code-block:: cpp

   g.provide("provide_numbers", provide_numbers, concurrency::unlimited)
     .output_product(product_query{.creator = "input", .layer = "event", .suffix = "numbers"});

Binding rule:

*art* code that fetches data from outside the normal event-product flow and then
makes it available to later stages often becomes a ``provide`` node.

``transform``
^^^^^^^^^^^^^

Use ``transform`` when the callable consumes one or more products and returns a
new product.

This is the most common binding for an *art* ``EDProducer``. A current
registration shape looks like this:

.. code-block:: cpp

   m.transform("add", examples::add, concurrency::unlimited)
     .input_family(product_query{.creator = "input", .layer = layer, .suffix = "i"},
                   product_query{.creator = "input", .layer = layer, .suffix = "j"})
     .output_product_suffixes("sum");

Binding rule:

* ``event.getProduct(...)`` becomes ``.input_family(...)`` plus normal function
  arguments.
* ``event.put(...)`` becomes the callable return value plus
  ``.output_product_suffixes(...)``.
* producer state that is really configuration or helper setup becomes lambda
  capture or an object constructed during registration.

``fold``
^^^^^^^^

Use ``fold`` when many products from a lower layer must be accumulated into one
product at a higher layer.

This maps naturally from *art* code that spreads state across callbacks such as
``beginSubRun()``, ``produce()``, and ``endSubRun()``. In *Phlex*, the
accumulation pattern is described directly:

.. code-block:: cpp

   fold("MySum", accumulate, 0, "subrun", concurrency::serial)
     .input_family(pset.get<input_tag>("input_tag"));

The examples deck also shows the multithreaded version:

.. code-block:: cpp

   g.fold("add", add, concurrency::unlimited, "event")
     .input_family(product_query{.creator = "iota", .layer = "lower1", .suffix = "new_number"})
     .output_product_suffixes("sum1");

Binding rule:

* mutable module members such as ``sum_`` become explicit fold state,
* reset logic in ``beginSubRun()`` becomes the fold's initial value,
* per-event updates become the fold function, and
* final publication in ``endSubRun()`` becomes the fold output product.

The main conceptual win is that the reduction is explicit. There is no hidden
state machine spread across framework callbacks.

``unfold``
^^^^^^^^^^

Use ``unfold`` when one input product drives the creation of many products in a
lower layer.

This is the dual of a fold. Rather than accumulating many lower-layer products
into one higher-layer result, an unfold expands one product into many lower-layer
products. A current registration shape from the *Phlex* tree is:

.. code-block:: cpp

   g.unfold<iota>("iota", &iota::predicate, &iota::unfold, concurrency::unlimited, "lower1")
     .input_family(product_query{.creator = "input", .layer = "event", .suffix = "max_number"})
     .output_product_suffixes("new_number");

Binding rule:

*art* code that manually loops over a collection, creates per-item work units,
or implicitly expands one product into many downstream computations should often
be rewritten as an explicit ``unfold`` followed by later ``transform`` or
``fold`` stages.

``observe``
^^^^^^^^^^^

Use ``observe`` when the callable consumes products but does not publish a new
product.

Typical uses are:

* validation,
* logging,
* assertions in examples and tests, and
* terminal side effects such as writing externally managed output.

A current registration shape looks like this:

.. code-block:: cpp

   m.observe("verify", verify_sum, concurrency::unlimited)
     .input_family(product_query{.creator = "input", .layer = "job"},
                   product_query{.creator = "add", .layer = layer});

Binding rule:

*art* analyzers and producer-side validation code often become ``observe``
nodes. If no new product is emitted, ``observe`` is usually the right model.

Crosswalk from *art* Patterns
-----------------------------

The following table-of-rules is a useful starting point during migration.

* ``EDProducer`` that computes one output from explicit inputs:
  usually ``transform``.
* ``EDProducer`` that only exposes boundary data to the graph:
  usually ``provide``.
* ``EDProducer`` or ``SharedProducer`` that accumulates across events into a
  subrun- or run-level result: usually ``fold``.
* producer logic that expands one input into many lower-layer work items:
  usually ``unfold``.
* ``EDAnalyzer`` or validation-only logic: usually ``observe``.

These are not rigid categories. A single *art* module may need to be split into
multiple *Phlex* nodes. That is often a sign that the binding is becoming more
accurate, not less.

Worked Examples
---------------

Simple Transform: Power
^^^^^^^^^^^^^^^^^^^^^^^

The examples deck shows a minimal transform based on a pure function:

.. code-block:: cpp

   int power(int base, unsigned exponent)
   {
     int result = 1;
     for (unsigned i = 0; i < exponent; ++i) {
       result *= base;
     }
     return result;
   }

   class PowerProducer : public art::EDProducer {
   public:
     explicit PowerProducer(fhicl::ParameterSet const& pset)
       : input_token_{consumes<int>(pset.get<art::InputTag>("input_tag"))}
       , exponent_{pset.get<unsigned>("exponent")}
     {
       produces<int>();
     }

     void produce(art::Event& event) override
     {
       auto const& value = event.getProduct(input_token_);
       event.put(std::make_unique<int>(power(value, exponent_)));
     }

   private:
     art::ProductToken<int> input_token_;
     unsigned exponent_;
   };

In *art*, this appears as a producer with a consumed input token, an ``exponent``
member, a ``produce()`` callback, and an ``event.put(...)`` call. In *Phlex*,
the same computation is expressed directly as a transform:

.. code-block:: cpp

   PHLEX_REGISTER_ALGORITHMS(pset) {
     auto f = [exp = pset.get<unsigned>("exponent")](int x) {
       return power(x, exp);
     };
     transform("Power", f, concurrency::unlimited)
       .input_family(pset.get<input_tag>("input_tag"));
   }

The important change is not syntax. The important change is that the framework
callback protocol disappears and the data transformation becomes explicit.

Simple Fold: Accumulating Across a Subrun
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The examples deck also shows a classic accumulation pattern. In *art*, the code
keeps ``sum_`` as module state, resets it in ``beginSubRun()``, updates it in
``produce()``, and publishes it in ``endSubRun()``. In *Phlex*, that pattern is
captured directly as a fold:

.. code-block:: cpp

   void accumulate(int& current_sum, int value)
   {
     current_sum += value;
   }

   class SumAcrossSubRun : public art::EDProducer {
   public:
     explicit SumAcrossSubRun(fhicl::ParameterSet const& pset)
       : input_token_{consumes<int>(pset.get<art::InputTag>("input_tag"))}
     {
       produces<int, art::InSubRun>();
     }

     void beginSubRun(art::SubRun&)
     {
       sum_ = 0;
     }

     void produce(art::Event& event) override
     {
       auto const& value = event.getProduct(input_token_);
       accumulate(sum_, value);
     }

     void endSubRun(art::SubRun& subrun) override
     {
       subrun.put(std::make_unique<int>(sum_));
     }

   private:
     art::ProductToken<int> input_token_;
     int sum_ = 0;
   };

The corresponding *Phlex* form makes the reduction explicit:

.. code-block:: cpp

   void accumulate(int& current_sum, int value)
   {
     current_sum += value;
   }

   PHLEX_REGISTER_ALGORITHM(pset) {
     fold("MySum", accumulate, 0, "subrun", concurrency::serial)
       .input_family(pset.get<input_tag>("input_tag"));
   }

The binding is direct:

* ``art::ProductToken<int>`` plus ``event.getProduct(input_token_)`` maps to
  ``.input_family(...)`` and a normal fold input argument.
* ``sum_`` maps to the explicit fold state.
* ``beginSubRun()`` resetting ``sum_ = 0`` maps to the fold initial value
  ``0``.
* ``produce()`` calling ``accumulate(sum_, value)`` maps to the fold step
  function ``accumulate``.
* ``endSubRun()`` and ``subrun.put(...)`` map to the fold's subrun-level output
  product.

When the accumulation can run concurrently, *Phlex* expresses that at the
registration site instead of forcing the module author to manage locks inside the
algorithm.

Transform Plus Observe
^^^^^^^^^^^^^^^^^^^^^^

Some migrated code naturally becomes more than one node. The
``add_and_verify.cpp`` example first computes a product and then validates it:

.. code-block:: cpp

   m.transform("add", examples::add, concurrency::unlimited)
     .input_family(product_query{.creator = "input", .layer = layer, .suffix = "i"},
                   product_query{.creator = "input", .layer = layer, .suffix = "j"})
     .output_product_suffixes("sum");

   m.observe("verify", verify_sum, concurrency::unlimited)
     .input_family(product_query{.creator = "input", .layer = "job"},
                   product_query{.creator = "add", .layer = layer});

This is a good example of how *Phlex* encourages one node for data creation and
a separate node for checking or side effects.

Provider Plus Transform
^^^^^^^^^^^^^^^^^^^^^^^

The *Phlex* tests also show a common pattern where graph inputs are created by
providers and then consumed by transforms:

.. code-block:: cpp

   g.provide("provide_numbers", provide_numbers, concurrency::unlimited)
     .output_product(product_query{.creator = "input", .layer = "event", .suffix = "numbers"});

   g.transform("triple_numbers", triple, concurrency::unlimited)
     .input_family(product_query{.creator = "input", .layer = "event", .suffix = "numbers"})
     .output_product_suffixes("tripled");

This binding is useful when an *art* module both fetches boundary data and
performs physics logic. In *Phlex*, those concerns are often cleaner when split
into separate nodes.

Unfold Followed by Fold
^^^^^^^^^^^^^^^^^^^^^^^

The ``unfold`` tests in the *Phlex* repository show the expansion-and-reduction
pattern explicitly:

.. code-block:: cpp

   g.unfold<iota>("iota", &iota::predicate, &iota::unfold, concurrency::unlimited, "lower1")
     .input_family(product_query{.creator = "input", .layer = "event", .suffix = "max_number"})
     .output_product_suffixes("new_number");

   g.fold("add", add, concurrency::unlimited, "event")
     .input_family(product_query{.creator = "iota", .layer = "lower1", .suffix = "new_number"})
     .output_product_suffixes("sum1");

This is the explicit *Phlex* form of a pattern that is often buried inside a
single *art* module: create many pieces of work, process them, and reduce the
results.

Example Binding: Gauss Hit Finder
---------------------------------

The ``gauss_hit_finder`` example is a realistic ``transform`` binding.

Input: A ``std::vector<recob::Wire>`` product is supplied to the transformation.

Configuration: Runtime configuration is read from ``config`` and stored in a plain
``find_hits_with_gaussians_cfg`` object.

Helper objects: Supporting algorithm objects are constructed during registration:

* ``CandHitStandard`` instances,
* ``PeakFitterMrqdt``, and
* ``HitFilterAlg``.

These are then captured by the transformation lambda.

Transformation: The registered callable takes wire data and returns a
``std::vector<recob::Hit>`` by calling the extracted algorithm.

Output: The result is published with the ``hits`` product suffix.

Concrete Registration Shape
---------------------------

The example expresses the binding with a *Phlex* registration that looks like
this:

.. code-block:: cpp

   m.transform("find_hits_with_gaussians",
               [cfg = std::move(cfg),
                cand_hit_standard_vec = std::move(cand_hit_standard_vec),
                peak_fitter_mrqdt = std::move(peak_fitter_mrqdt),
                hit_filter_alg = std::move(hit_filter_alg)]
               (std::vector<recob::Wire> const& wires) {
                  return examples::find_hits_with_gaussians(cfg,
                                                            wires,
                                                            cand_hit_standard_vec,
                                                            *peak_fitter_mrqdt,
                                                            *hit_filter_alg);
              },
               concurrency::unlimited)
     .input_family(product_query{.creator = "wires", .layer = layer, .suffix = ""})
     .output_product_suffixes("hits");

This shows the main binding ideas clearly.

* Input declaration is explicit.
* Configuration is captured explicitly.
* Helper dependencies are captured explicitly.
* The transformation returns the output product directly.

Transitional Binding Issues
---------------------------

Some migrations will reach a valid intermediate state before every framework feature is available in *Phlex*.

The example in this repository includes several such transitional issues.

* geometry-dependent fields are currently stubbed,
* a temporary file-based provider is used for comparison-driven testing, and
* output sorting and printing are present to aid validation against the *art* implementation.

These are not failures of the binding.
They are temporary boundary conditions that should be documented openly and reduced over time.

Phlex Binding Checklist
-----------------------

Before considering a binding complete, verify the following.

1. The computation has been expressed with the appropriate higher-order
   function: ``provide``, ``transform``, ``fold``, ``unfold``, or ``observe``.
2. Event retrieval has been replaced by explicit input declarations.
3. Configuration has been converted into plain data.
4. Remaining service-like dependencies are explicit at registration time.
5. Layer boundaries and accumulation or expansion behavior are explicit.
6. Transitional issues are documented rather than hidden.
