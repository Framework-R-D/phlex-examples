# Phlex examples

This repository contains examples of the use of the [Phlex framework](https://github.com/framework-r-d/phlex).

## Building this repository

To use the code in this repository, first install it:

```console
$ git clone https://github.com/Framework-R-D/phlex-examples.git
```

Now create a build directory:
```console
$ mkdir build-phlex-examples
$ cd build-phlex-examples
```

The repository be easily built by activating [the environment you created when installing Phlex](https://github.com/Framework-R-D/phlex/blob/main/INSTALLATION.md#creating-and-installing-the-phlex-environment):

```console
$ spack env activate my-phlex-environment
$ spack load cmake
$ cmake ../phlex-example
$ make -j
```

## Running the code

You should now be able to run a simple `phlex` job:

### Job with C++ algorithms

```console
$ phlex -c ~/phlex-examples/test-cpp-workflow.jsonnet
Using configuration file: /.../phlex-examples/test-cpp-workflow.jsonnet
[2025-12-22 13:17:51.496] [info] Number of worker threads: 24
Registering FORM output module...
Configuration:
  output_file: output.root
  technology: ROOT_TTREE
FormOutputModule initialized
⋮
[2025-12-22 13:17:51.760] [info]
Processed layers:

  job
   │
   └ event: 10

[2025-12-22 13:17:51.760] [info] CPU time: 0.21204s  Real time: 0.26498s  CPU efficiency:  80.02%
[2025-12-22 13:17:51.760] [info] Max. RSS: 268.848 MB
```

### Job with Python algorithms
