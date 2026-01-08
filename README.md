# Phlex examples

This repository contains examples of the use of the [Phlex framework](https://github.com/framework-r-d/phlex).

## Building this repository

To use the code in this repository, first install it:

```console
git clone https://github.com/Framework-R-D/phlex-examples.git
```

Now create a build directory:
```console
mkdir build-phlex-examples
cd build-phlex-examples
```

The repository can now be easily built by activating [the environment you created when installing Phlex](https://github.com/Framework-R-D/phlex/blob/main/INSTALLATION.md#creating-and-installing-the-phlex-environment):

```console
spack env activate my-phlex-environment
spack load cmake gcc@14  # adjust if you used a different compiler for phlex
cmake ../phlex-examples
make -j 12  # choose a suitable number of jobs for your machine
```

## Running the code

### Add your build directory to `PHLEX_PLUGIN_PATH`

```console
# Make sure you do this from your build directory
export PHLEX_PLUGIN_PATH=${PWD}:${PHLEX_PLUGIN_PATH}
```

You should now be able to run a simple `phlex` job:

### Job with C++ algorithms

```console
phlex -c ../phlex-examples/test-cpp-workflow.jsonnet
```

The output should look like:

```console
Using configuration file: ../phlex-examples/test-cpp-workflow.jsonnet
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
   └ spill: 10

[2025-12-22 13:17:51.760] [info] CPU time: 0.21204s  Real time: 0.26498s  CPU efficiency:  80.02%
[2025-12-22 13:17:51.760] [info] Max. RSS: 268.848 MB
```

### Job with Python algorithms

To run a job that uses a Python algorithm, the `PYTHONPATH` environment variable must be adjusted to include the directory with the Python module:

```console
export PYTHONPATH=$(realpath ../phlex-examples):${PYTHONPATH}
phlex -c ../phlex-examples/test-py-workflow.jsonnet
```

The output should look like:

```console
Using configuration file: ../phlex-examples/test-py-workflow.jsonnet
[2025-12-22 13:50:07.999] [info] Number of worker threads: 24
[2025-12-22 13:50:08.013] [info]
Processed layers:

  job

[2025-12-22 13:50:08.013] [info] CPU time: 0.01022s  Real time: 0.01483s  CPU efficiency:  68.91%
[2025-12-22 13:50:08.013] [info] Max. RSS: 29.628 MB
```

> [!NOTE]
> For Phlex 0.1.0, Python data products:
> - Cannot be written to output files
> - Must belong to the `"job"` data layer
