{
  driver: {
    cpp: 'generate_layers',
    layers: {
      spill: { parent: 'job', total: 5, starting_number: 1 },
      // The 'number' child layer is created implicitly by the unfold.
    },
  },
  sources: {
    vector_source: {
      cpp: 'vector_source',
      layer: 'spill',
    },
  },
  modules: {
    split: {
      cpp: 'split_square_verify',
      parent_layer: 'spill',
      child_layer: 'number',
    },
  },
}
