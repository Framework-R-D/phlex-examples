{
  driver: {
    cpp: 'generate_layers',
    layers: {
      spill: { parent: 'job', total: 5, starting_number: 0 },
    },
  },
  sources: {
    wires_source: {
      cpp: 'wires_source',
      layer: 'spill',
    },
    cell_id_source: {
      cpp: 'find_hits_with_gaussians_cell_id_hof',
      layer: 'spill',
    },
  },
  modules: {

    unfolds_design4_cpp: import 'unfolds_design4.libsonnet',

    cand_hit_standard_cpp: import 'cand_hit_standard.libsonnet',

    find_hits_with_gaussians_design4_cpp:
      import 'find_hits_with_gaussians_design4.libsonnet',

    folds_design4_cpp: import 'folds_design4.libsonnet',

    print_hits_to_file_cpp:
      (import 'print_hits_to_file.libsonnet') + {
        creator: 'fold_hits_into_vector_design4',
        filename_prefix: 'hits_design4',
      },
  },
}
