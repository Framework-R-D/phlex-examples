// Default configuration of the cand_hit_derivative module.
{
  cpp: 'cand_hit_derivative_hof',

  // Input: wire_roi_data
  roi_creator: 'unfold_wire_design4',
  roi_layer: 'roi',

  min_delta_ticks: 0,
  min_delta_peaks: 2.0,
  min_hit_height: 1.0,
  num_intervening_ticks: 6,
}
