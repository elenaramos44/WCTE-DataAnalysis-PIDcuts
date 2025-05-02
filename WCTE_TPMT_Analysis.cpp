// WCTE_TPMT_Analysis.cpp

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TText.h>
#include <TLegend.h>
#include <TSystem.h>
#include <TGraph.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <BRB ROOT file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    TString base_name = gSystem->BaseName(filename.c_str());

    // Extract run number from filename (expecting 'R####')
    int run_number = 0;
    size_t rpos = base_name.Index("R");
    size_t spos = base_name.Index("S");
    if (rpos != kNPOS && spos != kNPOS && spos > rpos) {
        run_number = atoi(base_name.Data() + rpos + 1);
    }

    std::string output_pdf = Form("tpmt_analysis_plots_run%d.pdf", run_number);

    TFile* file = TFile::Open(filename.c_str());
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening BRB file: " << filename << std::endl;
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
    std::vector<float>* bl_tdc_times = nullptr;
    std::vector<int>* bl_tdc_ids = nullptr;

    tree->SetBranchAddress("hit_mpmt_card_ids", &hit_card_ids);
    tree->SetBranchAddress("hit_pmt_channel_ids", &hit_channel_ids);
    tree->SetBranchAddress("hit_pmt_times", &hit_times);
    tree->SetBranchAddress("beamline_pmt_tdc_times", &bl_tdc_times);
    tree->SetBranchAddress("beamline_pmt_tdc_ids", &bl_tdc_ids);

    const int hit_tdc_channels[4] = {12, 13, 14, 15};
    const int bl_tdc_channels[4] = {0, 1, 2, 3};
    const char* ch_names[4] = {"T0-0L", "T0-1L", "T0-0R", "T0-1R"};

    TH1D* h_hit_tdc[4];
    TH1D* h_bl_tdc[4];
    TH1D* h_hit_t0_avg = new TH1D("h_hit_t0_avg", "T0 Avg from Hit PMT;TDC Time (ns);Counts", 200, 2150, 2250);
    TH1D* h_bl_t0_avg = new TH1D("h_bl_t0_avg", "T0 Avg from Beamline;TDC Time (ns);Counts", 200, 0, 100);
    std::map<int, TH1D*> h_card_timing;

    for (int i = 0; i < 4; ++i) {
        h_hit_tdc[i] = new TH1D(Form("h_hit_tdc_ch%d", hit_tdc_channels[i]),
                                Form("Hit PMT - Card 131 Ch %d (%s)", hit_tdc_channels[i], ch_names[i]),
                                200, 2150, 2250);
        h_bl_tdc[i] = new TH1D(Form("h_bl_tdc_ch%d", bl_tdc_channels[i]),
                               Form("Beamline TDC - Ch %d (%s)", bl_tdc_channels[i], ch_names[i]),
                               200, 0, 100);
    }

    TGraph* g_peak = new TGraph();
    int point = 0;

    Long64_t nEntries = std::min(tree->GetEntries(), (Long64_t)5000);
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        double sum_hit = 0, sum_bl = 0;
        int count_hit = 0, count_bl = 0;

        for (size_t j = 0; j < hit_card_ids->size(); ++j) {
            int card = (*hit_card_ids)[j];
            int ch = (*hit_channel_ids)[j];
            double t = (*hit_times)[j];

            if (card == 131) {
                for (int k = 0; k < 4; ++k) {
                    if (ch == hit_tdc_channels[k]) {
                        h_hit_tdc[k]->Fill(t);
                        if (t > 2150 && t < 2250) {
                            sum_hit += t;
                            ++count_hit;
                        }
                    }
                }
            }

            if (card < 130) {
                if (!h_card_timing.count(card)) {
                    h_card_timing[card] = new TH1D(Form("h_card_%d", card), Form("Hit Time Card %d;Time (ns);Counts", card), 5000, 0, 5000);
                }
                h_card_timing[card]->Fill(t);
            }
        }

        for (size_t j = 0; j < bl_tdc_ids->size(); ++j) {
            int ch = (*bl_tdc_ids)[j];
            float t = (*bl_tdc_times)[j];
            for (int k = 0; k < 4; ++k) {
                if (ch == bl_tdc_channels[k]) {
                    h_bl_tdc[k]->Fill(t);
                    if (t < 100) {
                        sum_bl += t;
                        ++count_bl;
                    }
                }
            }
        }

        if (count_hit == 4) h_hit_t0_avg->Fill(sum_hit / 4.0);
        if (count_bl == 4) h_bl_t0_avg->Fill(sum_bl / 4.0);
    }

    for (const auto& [card, hist] : h_card_timing) {
        int max_bin = hist->GetMaximumBin();
        double peak = hist->GetXaxis()->GetBinCenter(max_bin);
        g_peak->SetPoint(point++, card, peak);
    }

    TCanvas* c = new TCanvas("c", "TPMT Analysis", 1000, 800);

    // Title page
    c->Clear();
    TText* t = new TText(0.5, 0.6, "TPMT Timing Analysis");
    t->SetTextAlign(22); t->SetTextSize(0.04); t->Draw();

    TText* tfile = new TText(0.5, 0.4, Form("Input File: %s", base_name.Data()));
    tfile->SetTextAlign(22); tfile->SetTextSize(0.03); tfile->Draw();

    c->Print((output_pdf + "(").c_str());

    c->Clear(); c->Divide(2, 2);
    for (int i = 0; i < 4; ++i) { c->cd(i+1); h_hit_tdc[i]->Draw(); }
    c->Print(output_pdf.c_str());

    c->Clear(); h_hit_t0_avg->Draw(); c->Print(output_pdf.c_str());

    c->Clear(); c->Divide(2, 2);
    for (int i = 0; i < 4; ++i) { c->cd(i+1); h_bl_tdc[i]->Draw(); }
    c->Print(output_pdf.c_str());

    c->Clear(); h_bl_t0_avg->Draw(); c->Print(output_pdf.c_str());

    c->Clear();
    bool first = true; int col = 1;
    for (auto& [card, hist] : h_card_timing) {
        hist->SetLineColor(col++); hist->SetLineWidth(2);
        hist->GetXaxis()->SetRangeUser(0, 4000);
        hist->SetMaximum(10000);
        if (first) {
            hist->Draw("hist");
            first = false;
        } else {
            hist->Draw("hist same");
        }
    }
    c->Print(output_pdf.c_str());

    c->Clear();
    g_peak->SetTitle("Card vs Peak Time;Card ID;Peak Time (ns)");
    g_peak->SetMarkerStyle(20);
    g_peak->GetXaxis()->SetLimits(-10, 130);
    g_peak->SetMinimum(0);
    g_peak->SetMaximum(3000);
    g_peak->Draw("AP");
    c->Print((output_pdf + ")").c_str());

    file->Close();
    return 0;
}
