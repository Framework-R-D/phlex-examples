Phlex Binding
=============

Once a component has gone through framework and domain logic separation and
algorithm extraction, the remaining task is to express it in *Phlex* terms.
This is the binding stage.

Binding is not a line-by-line rewrite of an *art* module. The extracted
algorithm already says what the computation is. Binding says how that
computation appears in the *Phlex* graph: what products it consumes, what it
produces, what layer it belongs to, and which node shape best describes it.

One *art* module does not necessarily become one *Phlex* node. A module that
mixed boundary input, algorithmic transformation, reduction, and validation
often becomes several nodes. That split is not a complication — it is the
point. Each node becomes simpler and its role more explicit.

The organizing questions change:

In *art*, the question is often "what happens when the framework calls
``produce()``, ``beginSubRun()``, ``endSubRun()``, or ``analyze()``?"

In *Phlex*, the questions are:

* what is the callable shape of the extracted algorithm?
* which data products are its explicit inputs?
* what layer does it run in?
* does it create, transform, accumulate, expand, or observe data?

Binding to Higher-Order Functions
----------------------------------

*Phlex* code is organized around a small set of higher-order registration
functions. The question to ask is not "which *art* class did this come from?"
but "what kind of computation is this?"

``provide``
^^^^^^^^^^^

Use ``provide`` when the callable creates a product without consuming another
*Phlex* product as input. This is the boundary adapter that makes some input
available to later nodes — typically not the extracted physics algorithm itself.

Typical uses:

* exposing configuration-derived or job-level data as products,
* introducing data that originates outside the graph (e.g. from files or
  external services).

Registration shape:

.. code-block:: cpp

   s.provide("provide_i", [](data_cell_index const& id) -> int { return id.number(); })
     .output_product("input", "i", layer);

   s.provide("provide_geometry",
             [geometry_name](phlex::data_cell_index const& /* job */) -> examples::geometry
             {
               return examples::geometry{geometry_name};
             })
     .output_product("input", "geometry", "job");

*art* code that reaches outside the normal event-product flow to obtain input
and makes it available to later computations often becomes a ``provide`` node.
When an original *art* module mixed boundary acquisition with algorithmic work,
those two concerns are usually cleaner as a ``provide`` node followed by a
``transform`` node.

``transform``
^^^^^^^^^^^^^

Use ``transform`` when the callable consumes one or more products and returns
a new product. This is the most common binding for an extracted algorithm that
used to live in an *art* ``EDProducer``.

Registration shape:

.. code-block:: cpp

   m.transform("add", examples::add, concurrency::unlimited)
     .input_family(product_selector{.creator = "input", .layer = layer, .suffix = "i"},
                   product_selector{.creator = "input", .layer = layer, .suffix = "j"})
     .output_product_suffixes("sum");

Binding rule:

* ``event.getProduct(...)`` becomes ``.input_family(...)`` plus ordinary
  function arguments,
* ``event.put(...)`` becomes the callable return value plus the output
  declaration, and
* module members that are configuration or helper setup become lambda capture
  or objects constructed during registration.

When the extracted computation is already a normal function with explicit
arguments and a returned result, the binding is usually direct.

``fold``
^^^^^^^^

Use ``fold`` when many lower-layer products must be accumulated into one
product at a higher layer. This is the natural binding for code that used to
spread a reduction across callbacks such as ``beginSubRun()``, ``produce()``,
and ``endSubRun()``.

The step function shape is:

.. code-block:: cpp

   void step(State& state, Input const& value);

Registration shape:

.. code-block:: cpp

   g.fold("run_add", add, 0, concurrency::unlimited, "run")
     .input_family(
       product_selector{.creator = "input", .layer = "event", .suffix = "number"}
     )
     .output_product_suffixes("run_sum");

Binding rule:

* mutable module members become explicit fold state,
* reset logic in ``beginSubRun()`` becomes the fold initial value (here ``0``),
* per-event updates become the fold step function, and
* final publication in ``endSubRun()`` becomes the fold output product.

The fold target layer is declared directly at the registration site. There is
no hidden state machine spread across callbacks.

``unfold``
^^^^^^^^^^

Use ``unfold`` when one input product drives the creation of many products in
a lower layer. This is the right model when a former *art* module implicitly
expanded one product into many work items by looping over a collection or
splitting a data block inside one callback.

The unfold class provides:

.. code-block:: cpp

   Value initial_value() const;
   bool predicate(Value) const;
   std::pair<Value, Product> unfold(Value) const;

Registration shape:

.. code-block:: cpp

   g.unfold<iota>("iota", &iota::predicate, &iota::unfold, concurrency::unlimited, "lower1")
     .input_family(
       product_selector{.creator = "input", .layer = "event", .suffix = "max_number"}
     )
     .output_product_suffixes("new_number");

*art* code that manually expands one product into many downstream pieces of
work is often clearer as an explicit ``unfold``, usually followed by a
``transform`` or ``fold`` on the lower-layer products.

``observe``
^^^^^^^^^^^

Use ``observe`` when the callable consumes products but does not publish a new
product.

Typical uses:

* validation,
* assertions in examples and tests, and
* terminal side effects such as writing externally managed output.

Registration shape:

.. code-block:: cpp

   m.observe("verify", verify_sum, concurrency::unlimited)
     .input_family(product_selector{.creator = "input", .layer = "job"},
                   product_selector{.creator = "add", .layer = layer});

*art* analyzers, validation-only code, and producer-side checks that do not
emit new products often become ``observe`` nodes.

Crosswalk from Extracted *art* Designs
----------------------------------------

+---------------------------------------------------+-------------------+
| Extracted design                                  | *Phlex* node      |
+===================================================+===================+
| Function: computes one output from explicit inputs| ``transform``     |
+---------------------------------------------------+-------------------+
| Boundary adapter: introduces data from outside    | ``provide``       |
| the graph (files, services, configuration)        |                   |
+---------------------------------------------------+-------------------+
| Callback-driven accumulation across events,       | ``fold``          |
| subruns, or runs                                  |                   |
+---------------------------------------------------+-------------------+
| Loop that expands one input into many work items  | ``unfold``        |
+---------------------------------------------------+-------------------+
| Validation or terminal side-effect logic          | ``observe``       |
+---------------------------------------------------+-------------------+

These are not rigid categories. When one former *art* module becomes several
*Phlex* nodes, that is often a sign that the binding is exposing the true
dataflow rather than hiding it.

Worked Examples
---------------

Transform: Extracted Producer Algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The simplest binding is an extracted producer algorithm with a clean function
signature.

The extracted computation:

.. code-block:: cpp

   int power(int base, unsigned exponent)
   {
     int result = 1;
     for (unsigned i = 0; i < exponent; ++i) {
       result *= base;
     }
     return result;
   }

An equivalent *art* module:

.. code-block:: cpp

   class PowerProducer : public art::EDProducer {
   public:
     explicit PowerProducer(fhicl::ParameterSet const& pset)
       : input_token_{consumes<int>(pset.get<art::InputTag>("input"))}
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

In *Phlex*, the extracted algorithm binds directly as a transform:

.. code-block:: cpp

   PHLEX_REGISTER_ALGORITHMS(m, pset)
   {
     using namespace phlex;

     auto f = [exp = pset.get<unsigned>("exponent")](int x) {
       return power(x, exp);
     };
     m.transform("Power", f, concurrency::unlimited)
       .input_family(pset.get<product_selector>("input"));
   }

The callback protocol disappears. The computation appears directly as a
function from input product to output product.

Provide Plus Transform: Splitting Boundary Input from Algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When an original *art* module mixed boundary retrieval with algorithmic work,
the *Phlex* binding often becomes two nodes.

The ``gauss_hit_finder`` example in this repository has that shape. The
boundary adapter reads wire data from files (a transitional testing mechanism)
and makes it available as a graph product:

.. code-block:: cpp

   s.provide(
     "provide_wires",
     [](data_cell_index const& id) -> std::vector<recob::Wire> {
       auto const filename = std::format("wires_{}.dat", id.number());
       if (auto wires = read_wires_from_file(filename)) {
         return *wires;
       }
       throw std::runtime_error("Failure while reading from file: " + filename);
     }
    )
    .output_product("wires", "", layer);

The extracted hit-finding algorithm then binds as a transform:

.. code-block:: cpp

   m.transform("find_hits_with_gaussians",
               [cfg = std::move(cfg),
                cand_hits = std::move(cand_hit_standard_vec),
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
     .input_family(product_selector{.creator = "wires", .layer = layer, .suffix = ""})
     .output_product_suffixes("hits");

The split is explicit:

* boundary acquisition is a ``provide`` node,
* algorithmic work is a ``transform`` node, and
* the extracted function is unchanged.

Fold: Callback-State Accumulation Becomes Dataflow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Some *art* modules are not primarily transforms. Their real behavior is a
reduction across many lower-layer products, scattered across framework
callbacks.

A classic subrun accumulator in *art*:

.. code-block:: cpp

   void accumulate(int& current_sum, int value)
   {
     current_sum += value;
   }

   class SumAcrossSubRun : public art::EDProducer {
   public:
     explicit SumAcrossSubRun(fhicl::ParameterSet const& pset)
       : input_token_{consumes<int>(pset.get<art::InputTag>("input"))}
     {
       produces<int, art::InSubRun>();
     }

     void beginSubRun(art::SubRun&) { sum_ = 0; }

     void produce(art::Event& event) override
     {
       accumulate(sum_, event.getProduct(input_token_));
     }

     void endSubRun(art::SubRun& subrun) override
     {
       subrun.put(std::make_unique<int>(sum_));
     }

   private:
     art::ProductToken<int> input_token_;
     int sum_ = 0;
   };

In *Phlex*, the same step function binds as a fold:

.. code-block:: cpp

   PHLEX_REGISTER_ALGORITHMS(m, pset)
   {
     using namespace phlex;

     m.fold("MySum", accumulate, 0, "subrun", concurrency::serial)
       .input_family(pset.get<product_selector>("input"));
   }

The mapping is direct:

* ``sum_`` becomes explicit fold state,
* ``beginSubRun()`` resetting ``sum_ = 0`` becomes the fold initial value,
* ``produce()`` calling ``accumulate(sum_, value)`` becomes the fold step, and
* ``endSubRun()`` publishing the sum becomes the fold output product.

Observe: Validation Becomes a Declared Node
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Validation logic often survives extraction. In *Phlex* it becomes an explicit
node with declared inputs, rather than a comment inside a producer or a
separate unrelated analyzer callback.

.. code-block:: cpp

   m.transform("add", examples::add, concurrency::unlimited)
     .input_family(product_selector{.creator = "input", .layer = layer, .suffix = "i"},
                   product_selector{.creator = "input", .layer = layer, .suffix = "j"})
     .output_product_suffixes("sum");

   m.observe("verify", verify_sum, concurrency::unlimited)
     .input_family(product_selector{.creator = "input", .layer = "job"},
                   product_selector{.creator = "add", .layer = layer});

Unfold Plus Downstream Nodes: Making Fan-Out Explicit
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A common weakness in migrations is keeping a nested loop inside one producer
callback. When the real computation is "split this input into many work items,
process them, and reduce the results," *Phlex* should express that structure
explicitly as separate nodes.

In *art*, such code often looks like this:

.. code-block:: cpp

   void produce(art::Event& event) override
   {
     auto const max_number = event.getProduct<unsigned>(input_token_);
     unsigned sum = 0;
     for (unsigned i = 0; i != max_number; ++i) {
       sum += process_one(i);
     }
     event.put(std::make_unique<unsigned>(sum));
   }

After extraction, there are three separate pieces of work: generate the
per-item work units, process each one, and reduce them. In *Phlex*:

.. code-block:: cpp

   g.unfold<iota>("iota", &iota::predicate, &iota::unfold, concurrency::unlimited, "lower1")
     .input_family(
       product_selector{.creator = "input", .layer = "event", .suffix = "max_number"}
     )
     .output_product_suffixes("new_number");

   g.fold("add", add, concurrency::unlimited, "event")
     .input_family(
       product_selector{.creator = "iota", .layer = "lower1", .suffix = "new_number"}
     )
     .output_product_suffixes("sum");

A pipeline with per-item processing between the expansion and reduction adds
a ``transform`` in the middle:

.. code-block:: cpp

   g.unfold<Splitter>(
      "splitter", &Splitter::predicate, &Splitter::unfold, concurrency::unlimited, "chunk"
     )
     .input_family(product_selector{.creator = "input", .layer = "event", .suffix = "data"})
     .output_product_suffixes("chunk_data");

   g.transform("process", process_chunk, concurrency::unlimited)
     .input_family(
       product_selector{.creator = "splitter", .layer = "chunk", .suffix = "chunk_data"}
     )
     .output_product_suffixes("processed");

   g.fold("reduce", accumulate, concurrency::unlimited, "event")
     .input_family(
       product_selector{.creator = "process", .layer = "chunk", .suffix = "processed"})
     .output_product_suffixes("result");

This makes explicit a structure that is often hidden inside one large *art*
module.

Transitional Binding Issues
----------------------------

Some migrations will reach a valid intermediate state before every framework
feature is available in *Phlex*.

The examples in this repository include several such transitional issues:

* geometry-dependent fields are currently stubbed,
* a temporary file-based provider is used for comparison-driven testing, and
* output sorting and printing are present to aid validation against the *art*
  implementation.

These are not failures of the binding. They are temporary boundary conditions
that should be documented openly and reduced over time.

Phlex Binding Checklist
-----------------------

Before considering a binding complete, verify the following.

1. The extracted computation is represented with the appropriate node shape:
   ``provide``, ``transform``, ``fold``, ``unfold``, or ``observe``.
2. Product retrieval from callbacks has been replaced by explicit input
   declarations.
3. Configuration and helper setup appear in registration code, not inside the
   extracted algorithm.
4. Boundary-only work is kept separate from reusable algorithm code.
5. The chosen layer matches the real scope of the computation: event, subrun,
   run, job, or a derived lower layer.
6. A former *art* module that contains several kinds of work has been split
   into several nodes rather than forced into one.
7. Transitional limitations are documented rather than hidden inside the
   node implementation.
