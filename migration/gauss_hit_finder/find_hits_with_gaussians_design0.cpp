// See README.md for some general comments about this example.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <numbers>
#include <numeric>
#include <utility>

#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"

#include "find_hits_with_gaussians_design0.hpp"
#include "copied_from_larsoft_minor_edits/ICandidateHitFinder.h"

namespace {

  // This is an edited copy of the TMath::Gaus function from ROOT, since we
  // don't want to depend on ROOT in this example.
  double Gaus(double x, double mean, double sigma, bool norm) {
    if (sigma == 0) return 1.e30;
    double arg = (x-mean)/sigma;
    // for |arg| > 39  result is zero in double precision
    if (arg < -39.0 || arg > 39.0) return 0.0;
    double res = std::exp(-0.5*arg*arg);
    if (!norm) return res;
    return res/(2.50662827463100024*sigma); //sqrt(2*Pi)=2.50662827463100024
  }
}

namespace examples {
  std::vector<recob::Hit> find_hits_with_gaussians_design0(find_hits_with_gaussians_design0_cfg const& cfg,
                                                   std::vector<recob::Wire> const& wires,
                                                   std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
                                                   PeakFitterMrqdt const& peak_fitter_mrqdt,
                                                   HitFilterAlg const& hit_filter_alg) {

    std::cout << "Finding hits with Gaussians." << std::endl;

    // auto const& wireReadoutGeom = art::ServiceHandle<geo::WireReadout const>()->Get();

    //store in a thread safe way
    struct hitstruct {
      recob::Hit hit_tbb;
    };

    tbb::concurrent_vector<hitstruct> hitstruct_vec;
    tbb::concurrent_vector<hitstruct> filthitstruct_vec;

    //#################################################
    //###    Set the charge determination method    ###
    //### Default is to compute the normalized area ###
    //#################################################
    std::function<double(double, double, double, double, int, int)> chargeFunc =
      [](double /* peakMean */,
         double peakAmp,
         double peakWidth,
         double areaNorm,
         int /* low */,
         int /* hi */) { return std::sqrt(2 * std::numbers::pi) * peakAmp * peakWidth / areaNorm; };

    //##############################################
    //### Alternative is to integrate over pulse ###
    //##############################################
    if (cfg.area_method == 0)
      chargeFunc = [](double peakMean,
                      double peakAmp,
                      double peakWidth,
                      double /* areaNorm */,
                      int low,
                      int hi) {
        double charge(0);
        for (int sigPos = low; sigPos < hi; sigPos++)
          charge += peakAmp * Gaus(sigPos, peakMean, peakWidth, false);
        return charge;
      };
      
    //##############################
    //### Looping over the wires ###
    //##############################
    tbb::parallel_for(
      static_cast<std::size_t>(0),
      wires.size(),
      [&](size_t wireIter) {
        // ####################################
        // ### Getting this particular wire ###
        // ####################################
        recob::Wire const* wire = &wires[wireIter];

        // --- Setting Channel Number and Signal type ---

        raw::ChannelID_t channel = wire->Channel();

        // TODO. The plane number should come from the Geometry.
        // That is not implemented yet... Below is the commented out
        // version of the code that worked with the art framework.
        geo::PlaneID::PlaneID_t plane = 0;

        // get the WireID for this hit
        // std::vector<geo::WireID> wids = wireReadoutGeom.ChannelToWire(channel);
        // for now, just take the first option returned from ChannelToWire
        // geo::WireID wid = wids[0];
        // We need to know the plane to look up parameters
        // geo::PlaneID::PlaneID_t plane = wid.Plane;

        // ----------------------------------------------------------
        // -- Setting the appropriate signal widths and thresholds --
        // --    for the right plane.      --
        // ----------------------------------------------------------

        // #################################################
        // ### Set up to loop over ROI's for this wire   ###
        // #################################################
        const recob::Wire::RegionsOfInterest_t& signalROI = wire->SignalROI();

        tbb::parallel_for(
          static_cast<std::size_t>(0),
          signalROI.n_ranges(),
          [&](size_t rangeIter) {
            const auto& range = signalROI.range(rangeIter);
            // ROI start time
            raw::TDCtick_t roiFirstBinTick = range.begin_index();

            // ###########################################################
            // ### Scan the waveform and find candidate peaks + merge  ###
            // ###########################################################

            examples::ICandidateHitFinder::HitCandidateVec hitCandidateVec;
            examples::ICandidateHitFinder::MergeHitCandidateVec mergedCandidateHitVec;

            cand_hit_standard.at(plane)->findHitCandidates(
              range, 0, channel, hitCandidateVec);
            cand_hit_standard.at(plane)->MergeHitCandidates(
              range, hitCandidateVec, mergedCandidateHitVec);

            // #######################################################
            // ### Lets loop over the pulses we found on this wire ###
            // #######################################################

            for (auto& mergedCands : mergedCandidateHitVec) {
              int startT = mergedCands.front().startTick;
              int endT = mergedCands.back().stopTick;

              // ### Putting in a protection in case things went wrong ###
              // ### In the end, this primarily catches the case where ###
              // ### a fake pulse is at the start of the ROI           ###
              if (endT - startT < 5) continue;

              // #######################################################
              // ### Clearing the parameter vector for the new Pulse ###
              // #######################################################

              // === Setting The Number Of Gaussians to try ===
              int nGausForFit = mergedCands.size();

              // ##################################################
              // ### Calling the function for fitting Gaussians ###
              // ##################################################
              double chi2PerNDF(0.);
              int NDF(1);
              /*stand alone
                reco_tool::IPeakFitter::PeakParamsVec peakParamsVec(nGausForFit);
                */
              examples::IPeakFitter::PeakParamsVec peakParamsVec;

              // #######################################################
              // ### If # requested Gaussians is too large then punt ###
              // #######################################################
              if (mergedCands.size() <= cfg.max_multi_hit) {
                peak_fitter_mrqdt.findPeakParameters(
                  range.data(), mergedCands, peakParamsVec, chi2PerNDF, NDF);

                // If the chi2 is infinite then there is a real problem so we bail
                if (!(chi2PerNDF < std::numeric_limits<double>::infinity())) {
                  chi2PerNDF = 2. * cfg.chi2_ndf;
                  NDF = 2;
                }
              }

              // #######################################################
              // ### If too large then force alternate solution      ###
              // ### - Make n hits from pulse train where n will     ###
              // ###   depend on the fhicl parameter fLongPulseWidth ###
              // ### Also do this if chi^2 is too large              ###
              // #######################################################
              if (mergedCands.size() > cfg.max_multi_hit || nGausForFit * chi2PerNDF > cfg.chi2_ndf) {
                int longPulseWidth = cfg.long_pulse_width_vec.at(plane);
                int nHitsThisPulse = (endT - startT) / longPulseWidth;

                if (nHitsThisPulse > cfg.long_max_hits_vec.at(plane)) {
                  nHitsThisPulse = cfg.long_max_hits_vec.at(plane);
                  longPulseWidth = (endT - startT) / nHitsThisPulse;
                }

                if (nHitsThisPulse * longPulseWidth < endT - startT) nHitsThisPulse++;

                int firstTick = startT;
                int lastTick = std::min(firstTick + longPulseWidth, endT);

                peakParamsVec.clear();
                nGausForFit = nHitsThisPulse;
                NDF = 1.;
                chi2PerNDF = chi2PerNDF > cfg.chi2_ndf ? chi2PerNDF : -1.;

                for (int hitIdx = 0; hitIdx < nHitsThisPulse; hitIdx++) {
                  // This hit parameters
                  double ROIsumADC =
                    std::accumulate(range.begin() + firstTick, range.begin() + lastTick, 0.);
                  double peakSigma = (lastTick - firstTick) / 3.;  // Set the width...
                  double peakAmp = 0.3989 * ROIsumADC / peakSigma; // Use gaussian formulation
                  double peakMean = (firstTick + lastTick) / 2.;

                  // Store hit params
                  examples::IPeakFitter::PeakFitParams_t peakParams;

                  peakParams.peakCenter = peakMean;
                  peakParams.peakCenterError = 0.1 * peakMean;
                  peakParams.peakSigma = peakSigma;
                  peakParams.peakSigmaError = 0.1 * peakSigma;
                  peakParams.peakAmplitude = peakAmp;
                  peakParams.peakAmplitudeError = 0.1 * peakAmp;

                  peakParamsVec.push_back(peakParams);

                  // set for next loop
                  firstTick = lastTick;
                  lastTick = std::min(lastTick + longPulseWidth, endT);
                }
              }

              // #######################################################
              // ### Loop through returned peaks and make recob hits ###
              // #######################################################

              int numHits(0);

              // Make a container for what will be the filtered collection
              std::vector<recob::Hit> filteredHitVec;

              float nextpeak(0);
              float prevpeak(0);
              float nextpeakSig(0);
              float prevpeakSig(0);
              float nsigmaADC(2.0);
              float newright(0);
              float newleft(0);
              for (const auto& peakParams : peakParamsVec) {
                // Extract values for this hit
                float peakAmp = peakParams.peakAmplitude;
                float peakMean = peakParams.peakCenter;
                float peakWidth = peakParams.peakSigma;

                //std::cout<<" ans hits "<<numHits<<" gaus "<<nGausForFit<<std::endl;

                //ANS get prev and next
                if (numHits == 0) {
                  newleft = -9999;
                  newright = 9999;
                  nextpeak = 0;
                  prevpeak = 0;
                  nextpeakSig = 0;
                  prevpeakSig = 0;
                }
                if (numHits < nGausForFit - 1) {
                  nextpeak = (peakParamsVec.at(numHits + 1)).peakCenter;
                  nextpeakSig = (peakParamsVec.at(numHits + 1)).peakSigma;
                  //std::cout<<" ans size "<<peakParamsVec.size()<<" hit "<<numHits<<" next peak "<<nextpeak<<" sig "<<nextpeakSig<<std::endl;
                }
                if (numHits > 0) {
                  prevpeak = (peakParamsVec.at(numHits - 1)).peakCenter;
                  prevpeakSig = (peakParamsVec.at(numHits - 1)).peakSigma;
                  //std::cout<<" ans size "<<peakParamsVec.size()<<"hit "<<numHits<<" prev peak "<<prevpeak<<" sig "<<prevpeakSig<<std::endl;
                }

                // Place one bit of protection here
                if (std::isnan(peakAmp)) {
                  std::cout << "**** hit peak amplitude is a nan! Channel: " << channel
                            << ", start tick: " << startT << std::endl;
                  continue;
                }

                // Extract errors
                float peakAmpErr = peakParams.peakAmplitudeError;
                float peakMeanErr = peakParams.peakCenterError;
                float peakWidthErr = peakParams.peakSigmaError;

                // ### Charge ###
                float charge =
                  chargeFunc(peakMean, peakAmp, peakWidth, cfg.area_norms_vec[plane], startT, endT);
                ;
                float chargeErr =
                  std::sqrt(std::numbers::pi) * (peakAmpErr * peakWidthErr + peakWidthErr * peakAmpErr);

                // ### limits for getting sums
                std::vector<float>::const_iterator sumStartItr = range.begin() + startT;
                std::vector<float>::const_iterator sumEndItr = range.begin() + endT;

                //### limits for the sum of the Hit based on the gaussian peak and sigma
                std::vector<float>::const_iterator HitsumStartItr =
                  range.begin() + peakMean - nsigmaADC * peakWidth;
                std::vector<float>::const_iterator HitsumEndItr =
                  range.begin() + peakMean + nsigmaADC * peakWidth;

                if (nGausForFit > 1) {
                  if (numHits > 0) {
                    if ((peakMean - nsigmaADC * peakWidth) < (prevpeak + nsigmaADC * prevpeakSig)) {
                      float difPeak = peakMean - prevpeak;
                      float weightpeak = prevpeakSig / (prevpeakSig + peakWidth);
                      HitsumStartItr = range.begin() + prevpeak + difPeak * weightpeak;
                      newleft = prevpeak + difPeak * weightpeak;
                    }
                  }

                  if (numHits < nGausForFit - 1) {
                    if ((peakMean + nsigmaADC * peakWidth) > (nextpeak - nsigmaADC * nextpeakSig)) {
                      float difPeak = nextpeak - peakMean;
                      float weightpeak = peakWidth / (nextpeakSig + peakWidth);
                      HitsumEndItr = range.begin() + peakMean + difPeak * weightpeak;
                      newright = peakMean + difPeak * weightpeak;
                    }
                  }
                }

                //protection to avoid negative ranges
                if (newright - newleft < 0) continue;

                //avoid ranges out of ROI if it happens
                if (HitsumStartItr < sumStartItr) HitsumStartItr = sumStartItr;

                if (HitsumEndItr > sumEndItr) HitsumEndItr = sumEndItr;

                if (HitsumStartItr > HitsumEndItr) continue;

                // ### Sum of ADC counts
                double ROIsumADC = std::accumulate(sumStartItr, sumEndItr, 0.);
                double HitsumADC = std::accumulate(HitsumStartItr, HitsumEndItr, 0.);

                recob::Hit hit(wire->Channel(),
                               startT + roiFirstBinTick,
                               endT + roiFirstBinTick,
                               peakMean + roiFirstBinTick,
                               peakMeanErr,
                               peakWidth,
                               peakAmp,
                               peakAmpErr,
                               ROIsumADC,
                               HitsumADC,
                               charge,
                               chargeErr,
                               nGausForFit,
                               numHits,
                               chi2PerNDF,
                               NDF,
                               wire->View(),
                               // Geometry system for Phlex is not yet implemented,
                               // so we just set signal type to 0 (kInduction) for now.
                               geo::kInduction,
                               // art::ServiceHandle<geo::WireReadout const>()->Get().SignalType(wire.Channel()),
                               // wid also comes from the Geometry, which is not yet implemented, so we just set
                               // it to a default value for now.
                               geo::WireID());
                               // wid);

                if (cfg.filter_hits) filteredHitVec.push_back(hit);

                // This loop will store ALL hits
                hitstruct tmp{std::move(hit)};
                hitstruct_vec.push_back(std::move(tmp));

                numHits++;
              } // <---End loop over gaussians

              // Should we filter hits?
              if (cfg.filter_hits && !filteredHitVec.empty()) {
                // #######################################################################
                // Is all this sorting really necessary?  Would it be faster to just loop
                // through the hits and perform simple cuts on amplitude and width on a
                // hit-by-hit basis, either here in the module (using fPulseHeightCuts and
                // fPulseWidthCuts) or in HitFilterAlg?
                // #######################################################################

                // Sort in ascending peak height
                // (I believe the preceding comment is incorrect. The sort below
                // is in descending order of peak height, not ascending.)
                std::sort(filteredHitVec.begin(),
                          filteredHitVec.end(),
                          [](const auto& left, const auto& right) {
                            return left.PeakAmplitude() > right.PeakAmplitude();
                          });

                // Reject if the first hit fails the PH/wid cuts
                if (filteredHitVec.front().PeakAmplitude() < cfg.pulse_height_cuts.at(plane) ||
                    filteredHitVec.front().RMS() < cfg.pulse_width_cuts.at(plane))
                  filteredHitVec.clear();

                // Now check other hits in the snippet
                if (filteredHitVec.size() > 1) {
                  // The largest pulse height will now be at the front...
                  float largestPH = filteredHitVec.front().PeakAmplitude();

                  // Find where the pulse heights drop below threshold
                  float threshold(cfg.pulse_ratio_cuts.at(plane));

                  std::vector<recob::Hit>::iterator smallHitItr =
                    std::find_if(filteredHitVec.begin(),
                                 filteredHitVec.end(),
                                 [largestPH, threshold](const auto& hit) {
                                   return hit.PeakAmplitude() < 8. &&
                                          hit.PeakAmplitude() / largestPH < threshold;
                                 });

                  // Shrink to fit
                  if (smallHitItr != filteredHitVec.end())
                    filteredHitVec.resize(std::distance(filteredHitVec.begin(), smallHitItr));

                  // Resort in time order
                  std::sort(filteredHitVec.begin(),
                            filteredHitVec.end(),
                            [](const auto& left, const auto& right) {
                              return left.PeakTime() < right.PeakTime();
                            });
                }

                // Copy the hits we want to keep to the filtered hit collection
                for (const auto& filteredHit : filteredHitVec) {
                  if (!cfg.filter_hits || hit_filter_alg.IsGoodHit(filteredHit)) {
                    hitstruct tmp{std::move(filteredHit)};
                    filthitstruct_vec.push_back(std::move(tmp));
                  }
                }
              }
            } //<---End loop over merged candidate hits
          }   //<---End looping over ROI's
        );    //end tbb parallel for
      }       //<---End looping over all the wires
    );        //end tbb parallel for

    std::vector<recob::Hit> hits;

    if (cfg.filter_hits) {
      for (size_t i = 0; i < filthitstruct_vec.size(); i++) {
        hits.emplace_back(filthitstruct_vec[i].hit_tbb);
      }
    } else {
      for (size_t i = 0; i < hitstruct_vec.size(); i++) {
        hits.emplace_back(hitstruct_vec[i].hit_tbb);
      }
    }

    return hits;
  } // End of find_hits_with_gaussians_design0
} // end of examples namespace
