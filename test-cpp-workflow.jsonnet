{
  driver: {
    cpp: 'generate_layers',
    layers: {
      event: { parent: "job", total: 10, starting_number: 1 }
    }
  },
  sources: {
    ij_source: {
      cpp: 'ij_source'
    }
  },
  modules: {
    add_cpp: {
      cpp: 'module',
    },
    # Write "sum" data product to output file
    output: {
      cpp: 'form_module',
      products: ["sum"]
    },
  },
}
