#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "WCTE_Utility.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <BRB ROOT file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    TFile* file = TFile::Open(filename.c_str());
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    TTree* tree = (TTree*)file->Get("WCTEReadoutWindows");
    if (!tree) {
        std::cerr << "Tree 'WCTEReadoutWindows' not found!" << std::endl;
        return 1;
    }

    std::vector<int>* hit_card_ids = nullptr;
    std::vector<int>* hit_channel_ids = nullptr;
    std::vector<double>* hit_times = nullptr;

    tree->SetBranchAddress("hit_mpmt_card_ids", &hit_card_ids);
    tree->SetBranchAddress("hit_pmt_channel_ids", &hit_channel_ids);
    tree->SetBranchAddress("hit_pmt_times", &hit_times);

    // -- Setup utility class
    WCTE_Utility util;
    util.SetHitPMTData(hit_card_ids, hit_channel_ids, hit_times);
    util.InitializeT0Calibration(tree, 1000);  // or however many events you want

    // -- Direct method setup
    const int t0_channels[4] = {12, 13, 14, 15};
    double t0_mean[4] = {0}, t0_sigma[4] = {0};

    // Estimate mean/sigma from first 1000 entries (same as utility)
    std::vector<double> t0_values[4];
    Long64_t entries_to_use = std::min(tree->GetEntries(), (Long64_t)1000);
    for (Long64_t i = 0; i < entries_to_use; ++i) {
        tree->GetEntry(i);
        for (size_t j = 0; j < hit_card_ids->size(); ++j) {
            int card = (*hit_card_ids)[j];
            if (card != 131) continue;
            int ch = (*hit_channel_ids)[j];
            double t = (*hit_times)[j];
            for (int k = 0; k < 4; ++k) {
                if (ch == t0_channels[k] && t > 2150 && t < 2250) {
                    t0_values[k].push_back(t);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        const auto& v = t0_values[i];
        if (v.empty()) continue;
        double sum = std::accumulate(v.begin(), v.end(), 0.0);
        double mean = sum / v.size();
        double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
        double sigma = std::sqrt(sq_sum / v.size() - mean * mean);
        t0_mean[i] = mean;
        t0_sigma[i] = sigma;
    }

    // -- Main loop: compute & compare T0 values
    Long64_t nEntries = std::min(tree->GetEntries(), (Long64_t)100);
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);

        double t0_sum = 0;
        int t0_hits = 0;
        for (size_t j = 0; j < hit_card_ids->size(); ++j) {
            int card = (*hit_card_ids)[j];
            int ch = (*hit_channel_ids)[j];
            double t = (*hit_times)[j];
            if (card != 131) continue;
            for (int k = 0; k < 4; ++k) {
                if (ch == t0_channels[k] && std::abs(t - t0_mean[k]) < 3 * t0_sigma[k]) {
                    t0_sum += t;
                    t0_hits++;
                }
            }
        }

        if (t0_hits != 4) continue;

        double ref_t0 = t0_sum / 4.0;


        auto util_t0_opt = util.ComputeEventT0();

        //if (util_t0_opt && std::abs(ref_t0 - *util_t0_opt) > 0.1) {
        //if (util_t0_opt) {
           std::cout << "Event " << i << ": T0 mismatch!"
                      << " Ref = " << ref_t0 << " ns, "
                      << " Util = " << *util_t0_opt << " ns" << std::endl;
        //}
    }

    file->Close();
    return 0;
}
