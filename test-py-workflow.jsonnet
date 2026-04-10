{
  driver: {
    cpp: 'generate_layers',
  },
  sources: {
    numbers_source: {
      cpp: 'numbers_source',
      layer: 'job',
    },
  },
  modules: {
    subtract: {
      py: 'subtract',
      input: [
        { creator: 'input', suffix: 'i', layer: 'job' },
        { creator: 'input', suffix: 'j', layer: 'job' },
      ],
      output: ['difference'],
    },
    // Python data products cannot yet be written to output files
    //   output: {
    //     cpp: 'form_module',
    //     products: [],
    //   },
  },
}
