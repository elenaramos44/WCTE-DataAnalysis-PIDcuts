#ifndef WCTE_UTILITY_H
#define WCTE_UTILITY_H

#include <vector>
#include <optional>
#include <numeric> 
#include <TTree.h>      // ← This is required
#include <TH1D.h>
#include <TF1.h>
#include <TMath.h>


class WCTE_Utility {
public:
    WCTE_Utility();

    void SetHitPMTData(const std::vector<int>* card_ids,
                       const std::vector<int>* channel_ids,
                       const std::vector<double>* times);

    void InitializeT0Calibration(TTree* tree, size_t n_events);
    std::optional<double> ComputeEventT0() const;  // Computes per-event average T0 using stored cuts

private:
    const std::vector<int>* card_ids_ = nullptr;
    const std::vector<int>* channel_ids_ = nullptr;
    const std::vector<double>* times_ = nullptr;

    static constexpr int target_card_ = 131;
    static constexpr int t0_channels_[4] = {12, 13, 14, 15};
    double t0_mean_[4] = {0};
    double t0_sigma_[4] = {0};
    bool initialized_ = false;
};

#endif
