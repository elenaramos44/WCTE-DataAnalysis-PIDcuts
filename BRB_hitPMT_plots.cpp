#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TText.h>
#include <iostream>
#include <vector>
#include <map>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <BRB root file>" << std::endl;
        return 1;
    }

    TFile* fileBRB = TFile::Open(argv[1]);
    if (!fileBRB || fileBRB->IsZombie()) {
        std::cerr << "Error opening BRB file!" << std::endl;
        return 1;
    }

    TTree* treeBRB = (TTree*)fileBRB->Get("WCTEReadoutWindows");
    if (!treeBRB) {
        std::cerr << "Error: Could not find 'WCTEReadoutWindows' tree!" << std::endl;
        return 1;
    }

    // Setup HIT PMT branches
    std::vector<float>* hit_qdc = nullptr;
    std::vector<double>* hit_tdc = nullptr;
    std::vector<int>* hit_card = nullptr;
    std::vector<int>* hit_chan = nullptr;

    treeBRB->SetBranchAddress("hit_pmt_charges", &hit_qdc);
    treeBRB->SetBranchAddress("hit_pmt_times", &hit_tdc);
    treeBRB->SetBranchAddress("hit_mpmt_card_ids", &hit_card);
    treeBRB->SetBranchAddress("hit_pmt_channel_ids", &hit_chan);

    Long64_t nEntriesBRB = treeBRB->GetEntries();
    nEntriesBRB = 5000; // Limit to 5000 entries for testing

    // Mapping: (card, channel) -> detector name
    std::map<std::pair<int, int>, std::string> id_name_map = {
        {{130,0}, "ACT0-L"}, {{130,1}, "ACT0-R"}, {{130,2}, "ACT1-L"}, {{130,3}, "ACT1-R"},
        {{130,5}, "ACT2-L"}, {{130,6}, "ACT2-R"}, {{130,7}, "ACT3-L"}, {{130,8}, "ACT3-R"},
        {{130,9}, "ACT4-L"}, {{130,10},"ACT4-R"}, {{130,11},"ACT5-L"}, {{130,12},"ACT5-R"},
        {{130,13},"T1-0L"}, {{130,14},"T1-0R"}, {{130,15},"T1-1L"}, {{130,16},"T1-1R"},
        {{130,17},"HC-0"}, {{130,18},"HC-1"}, {{130,19},"Trigger-130"},
        {{131,0}, "Trigger-131"}, {{131,9}, "Laser"}, {{131,10},"T2"}, {{131,11},"T3"},
        {{131,12},"T0-0L"}, {{131,13},"T0-0R"}, {{131,14},"T0-1L"}, {{131,15},"T0-1R"},
        {{131,17},"PbG"}, {{131,18},"MuL"}, {{131,19},"MuR"},
        {{132,0}, "TOF-0"}, {{132,1}, "TOF-1"}, {{132,2}, "TOF-2"}, {{132,3}, "TOF-3"},
        {{132,4}, "TOF-4"}, {{132,5}, "TOF-5"}, {{132,6}, "TOF-6"}, {{132,7}, "TOF-7"},
        {{132,8}, "TOF-8"}, {{132,10},"TOF-9"}, {{132,11},"TOF-A"}, {{132,12},"TOF-B"},
        {{132,13},"TOF-C"}, {{132,14},"TOF-D"}, {{132,15},"TOF-E"}, {{132,16},"TOF-F"},
        {{132,17},"T4-L"}, {{132,18},"T4-R"}, {{132,19},"Trigger-132"}
    };

    std::map<std::pair<int, int>, TH1D*> hists_qdc;
    std::map<std::pair<int, int>, TH1D*> hists_tdc;

    for (Long64_t i = 0; i < nEntriesBRB; ++i) {
        treeBRB->GetEntry(i);

        if (!hit_qdc || !hit_tdc || !hit_card || !hit_chan) continue;

        for (size_t j = 0; j < hit_qdc->size(); ++j) {
            int card = (*hit_card)[j];
            int chan = (*hit_chan)[j];

            if (card != 130 && card != 131 && card != 132) continue; // Only cards 130, 131, 132

            auto key = std::make_pair(card, chan);

            if (!hists_qdc.count(key)) {
                hists_qdc[key] = new TH1D(Form("hQDC_card%d_chan%d", card, chan), 
                                          Form("QDC: Card %d Chan %d;QDC;Counts", card, chan),
                                          8500, 0, 8500);
            }

            if (!hists_tdc.count(key)) {
                hists_tdc[key] = new TH1D(Form("hTDC_card%d_chan%d", card, chan), 
                                          Form("TDC: Card %d Chan %d;TDC (ns);Counts", card, chan),
                                          8500, 0, 8500);
            }

            hists_qdc[key]->Fill((*hit_qdc)[j]);
            hists_tdc[key]->Fill((*hit_tdc)[j]);
        }
    }

    // Now draw everything
    TCanvas* c = new TCanvas("c", "Hit PMT Distributions", 1000, 1200);
    c->Print("hit_pmt_detector_plots.pdf("); // Open PDF

    for (auto& [key, hqdc] : hists_qdc) {
        int card = key.first;
        int chan = key.second;
        auto htdc = hists_tdc[key];

        c->Clear();
        c->Divide(1,2);

        c->cd(1);
        hqdc->Draw();

        c->cd(2);
        htdc->Draw();

        // Add a title to the top
        if (id_name_map.count(key)) {
            c->cd();
            TText* t = new TText(0.5, 0.95, Form("Detector: %s", id_name_map[key].c_str()));
            t->SetTextAlign(22);
            t->SetTextSize(0.03);
            t->Draw();
        }

        c->Print("hit_pmt_detector_plots.pdf");
    }

    c->Print("hit_pmt_detector_plots.pdf)"); // Close PDF

    fileBRB->Close();
    return 0;
}
