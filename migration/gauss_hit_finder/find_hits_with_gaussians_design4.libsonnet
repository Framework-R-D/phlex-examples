{
  cpp: 'find_hits_with_gaussians_design4_hof',

  layer_roi: 'roi',
  cand_hit_finder: 'cand_hit_standard',

  filter_hits: false,
  long_max_hits_vec: [1, 1, 1],
  long_pulse_width_vec: [16, 16, 16],
  max_multi_hit: 4,
  area_method: 0,
  area_norms_vec: [13.25, 13.25, 13.25],
  chi2_ndf: 50.0,
  pulse_height_cuts: [3.0, 3.0, 3.0],
  pulse_width_cuts: [2.0, 1.5, 1.0],
  pulse_ratio_cuts: [0.35, 0.40, 0.20],
  peak_fitter_mrqdt_config: {
    min_width: 0.5,
    max_width_mult: 3.0,
    peak_range_fact: 2.0,
    peak_amp_range: 2.0,
  },
  hit_filter_alg_config: {
    min_pulse_height: [5.0, 5.0, 5.0],
    min_pulse_sigma: [1.0, 1.0, 1.0],
  },
}
