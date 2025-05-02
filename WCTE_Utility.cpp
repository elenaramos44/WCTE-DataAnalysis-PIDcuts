#include "WCTE_Utility.h"
#include <TTree.h>      // ← This is required
#include <TH1D.h>
#include <TF1.h>
#include <TMath.h>
#include <iostream>


WCTE_Utility::WCTE_Utility() {}

void WCTE_Utility::SetHitPMTData(const std::vector<int>* card_ids,
                                 const std::vector<int>* channel_ids,
                                 const std::vector<double>* times) {
    card_ids_ = card_ids;
    channel_ids_ = channel_ids;
    times_ = times;
}

void WCTE_Utility::InitializeT0Calibration(TTree* tree, size_t n_events) {
    if (!tree || !card_ids_ || !channel_ids_ || !times_) return;

    std::vector<TH1D*> hists(4);
    for (int i = 0; i < 4; ++i) {
        hists[i] = new TH1D(Form("h_t0_ch%d", t0_channels_[i]),
                            Form("T0 Channel %d", t0_channels_[i]),
                            200, 2150, 2250);
    }

    size_t nEntries = std::min((size_t)tree->GetEntries(), n_events);
    for (size_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        for (size_t j = 0; j < card_ids_->size(); ++j) {
            if ((*card_ids_)[j] != target_card_) continue;

            int ch = (*channel_ids_)[j];
            double t = (*times_)[j];

            for (int k = 0; k < 4; ++k) {
                if (ch == t0_channels_[k] && t > 2150 && t < 2250) {
                    hists[k]->Fill(t);
                    break;
                }
            }
        }
    }

    for (int k = 0; k < 4; ++k) {
        TH1D* h = hists[k];
        double peak = h->GetXaxis()->GetBinCenter(h->GetMaximumBin());
        TF1* fit = new TF1(Form("fit_ch%d", t0_channels_[k]), "gaus", peak - 8, peak + 8);
        fit->SetParameters(h->GetMaximum(), peak, 4.0);
        h->Fit(fit, "RQ");

        t0_mean_[k] = fit->GetParameter(1);
        t0_sigma_[k] = fit->GetParameter(2);

        delete fit;
        delete h;
    }

    initialized_ = true;
}

std::optional<double> WCTE_Utility::ComputeEventT0() const {
    if (!card_ids_ || !channel_ids_ || !times_) return std::nullopt;

    double sum = 0.0;
    int count = 0;

    for (size_t i = 0; i < card_ids_->size(); ++i) {
        int card = (*card_ids_)[i];
        int ch   = (*channel_ids_)[i];
        double t = (*times_)[i];

        if (card != target_card_) continue;

        for (int k = 0; k < 4; ++k) {
            if (ch == t0_channels_[k]) {
                double mean  = t0_mean_[k];   // Corrected
                double sigma = t0_sigma_[k];  // Corrected

                if (std::abs(t - mean) < 3 * sigma) {
                    sum += t;
                    ++count;
                } else {
                    std::cout << "[DEBUG] Rejecting hit: ch=" << ch
                              << " t=" << t << " outside 3σ from mean=" << mean
                              << ", σ=" << sigma << std::endl;
                }
                break;
            }
        }
    }

    if (count == 4) return sum / 4.0;

    std::cout << "[DEBUG] Skipped event: only " << count << " valid T0 hits" << std::endl;
    return std::nullopt;
}
