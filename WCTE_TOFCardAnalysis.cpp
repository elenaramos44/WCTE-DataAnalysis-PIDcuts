// WCTE_TOFCardAnalysis.cpp

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TGraph.h>
#include <TF1.h>
#include <TLegend.h>
#include <TText.h>
#include <TSystem.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include "WCTE_BeamMon_PID.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <BRB ROOT file> <boxcuts.json>" << std::endl;
        return 1;
    }

    const int selected_card = 31;
    std::string filename = argv[1];
    std::string boxcutfile = argv[2];

    std::string base = gSystem->BaseName(filename.c_str());
    size_t pos1 = base.find("R");
    size_t pos2 = base.find("S");
    int run_id = (pos1 != std::string::npos && pos2 != std::string::npos && pos2 > pos1) ?
                 std::stoi(base.substr(pos1 + 1, pos2 - pos1 - 1)) : 0;

    TString output_pdf = Form("tof_qdc_analysis_run%d_card%d.pdf", run_id, selected_card);

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
    std::vector<float>* hit_qdc = nullptr;
    std::vector<float>* beam_qdc = nullptr;
    std::vector<int>* beam_qdc_ids = nullptr;
    std::vector<float>* beam_tdc = nullptr;
    std::vector<int>* beam_tdc_ids = nullptr;

    tree->SetBranchAddress("hit_mpmt_card_ids", &hit_card_ids);
    tree->SetBranchAddress("hit_pmt_channel_ids", &hit_channel_ids);
    tree->SetBranchAddress("hit_pmt_times", &hit_times);
    tree->SetBranchAddress("hit_pmt_charges", &hit_qdc);
    tree->SetBranchAddress("beamline_pmt_qdc_charges", &beam_qdc);
    tree->SetBranchAddress("beamline_pmt_qdc_ids", &beam_qdc_ids);
    tree->SetBranchAddress("beamline_pmt_tdc_times", &beam_tdc);
    tree->SetBranchAddress("beamline_pmt_tdc_ids", &beam_tdc_ids);

    WCTE_BeamMon_PID pid;
    pid.LoadBoxCuts(boxcutfile);
    pid.SetRunID(run_id);

    const int t0_ch[4] = {12, 13, 14, 15};
    TH1D* h_t0_ch[4];
    for (int i = 0; i < 4; ++i)
        h_t0_ch[i] = new TH1D(Form("h_t0_ch%d", t0_ch[i]), Form("Card 131 Ch %d;Time (ns);Counts", t0_ch[i]), 200, 2150, 2250);
    TH1D* h_selected_all = new TH1D("h_selected_all", "All Hit Times on Selected Card;Time (ns);Counts", 200, 1000, 5000);

    Long64_t nEntries = std::min(tree->GetEntries(), (Long64_t)5000);
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        for (size_t j = 0; j < hit_card_ids->size(); ++j) {
            int card = (*hit_card_ids)[j];
            int ch = (*hit_channel_ids)[j];
            double t = (*hit_times)[j];
            if (card == 131) {
                for (int k = 0; k < 4; ++k)
                    if (ch == t0_ch[k]) h_t0_ch[k]->Fill(t);
            }
            if (card == selected_card)
                h_selected_all->Fill(t);
        }
    }

    double t0_mean[4], t0_sigma[4];
    for (int i = 0; i < 4; ++i) {
        TF1* g = new TF1(Form("gfit_%d", i), "gaus", 2150, 2250);
        g->SetParameters(h_t0_ch[i]->GetMaximum(), 2200, 5);
        h_t0_ch[i]->Fit(g, "RQ");
        t0_mean[i] = g->GetParameter(1);
        t0_sigma[i] = g->GetParameter(2);
    }

    TH1D* h_tof = new TH1D("h_tof", "ToF (All);ToF (ns);Counts", 200, -1010, -970);
    TH1D* h_qdc = new TH1D("h_qdc", "QDC Sum (All);QDC;Counts", 2000, 0, 14000);
    TH2D* h_qdc_vs_tof = new TH2D("h_qdc_vs_tof", "QDC vs ToF (All);ToF (ns);QDC", 200, -1010, -970, 2000, 0, 14000);
    TH1D* h_tof_min = (TH1D*)h_tof->Clone("h_tof_min"); h_tof_min->SetTitle("ToF (All, Min Time)");
    TH1D* h_qdc_min = (TH1D*)h_qdc->Clone("h_qdc_min"); h_qdc_min->SetTitle("QDC Sum (All, Min Time)");
    TH2D* h_qdc_vs_tof_min = (TH2D*)h_qdc_vs_tof->Clone("h_qdc_vs_tof_min"); h_qdc_vs_tof_min->SetTitle("QDC vs ToF (Min Time)");

    std::map<int, TString> pid_names = { {11, "electron"}, {13, "muon"}, {211, "pion"} };
    std::map<int, int> pid_colors = { {11, kBlue}, {13, kRed}, {211, kGreen+2} };

    std::map<int, TH1D*> h_tof_pid, h_qdc_pid, h_qdc_pid_min, h_tof_pid_min;
    std::map<int, TH2D*> h_qdc_vs_tof_pid, h_qdc_vs_tof_pid_min;

    for (const auto& [pid_code, name] : pid_names) {
        h_tof_pid[pid_code] = new TH1D(Form("h_tof_%s", name.Data()), "", 200, -1010, -970);
        h_tof_pid[pid_code]->SetLineColor(pid_colors[pid_code]);
        h_tof_pid_min[pid_code] = new TH1D(Form("h_tof_min_%s", name.Data()), "", 200, -1010, -970);
        h_tof_pid_min[pid_code]->SetLineColor(pid_colors[pid_code]);

        h_qdc_pid[pid_code] = new TH1D(Form("h_qdc_%s", name.Data()), "", 2000, 0, 14000);
        h_qdc_pid[pid_code]->SetLineColor(pid_colors[pid_code]);
        h_qdc_pid_min[pid_code] = new TH1D(Form("h_qdc_min_%s", name.Data()), "", 2000, 0, 14000);
        h_qdc_pid_min[pid_code]->SetLineColor(pid_colors[pid_code]);

        h_qdc_vs_tof_pid[pid_code] = new TH2D(Form("h_qdc_vs_tof_%s", name.Data()), "", 200, -1010, -970, 2000, 0, 14000);
        h_qdc_vs_tof_pid_min[pid_code] = new TH2D(Form("h_qdc_vs_tof_min_%s", name.Data()), "", 200, -1010, -970, 2000, 0, 14000);
    }

    nEntries = std::min(tree->GetEntries(), (Long64_t)500000);
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        pid.SetBeamlineData(beam_qdc, beam_qdc_ids, beam_tdc, beam_tdc_ids);
        int pid_code = pid.GetParticleID();

        double t0_sum = 0; int t0_hits = 0;
        double card_sum = 0; int card_hits = 0;
        double qdc_sum = 0;
        double min_time = 1e9;

        for (size_t j = 0; j < hit_card_ids->size(); ++j) {
            int card = (*hit_card_ids)[j];
            int ch = (*hit_channel_ids)[j];
            double t = (*hit_times)[j];
            double q = (*hit_qdc)[j];

            if (card == 131) {
                for (int k = 0; k < 4; ++k)
                    if (ch == t0_ch[k] && fabs(t - t0_mean[k]) < 3 * t0_sigma[k]) {
                        t0_sum += t;
                        t0_hits++;
                    }
            }

            if (card == selected_card && t > 1000 && t < 5000) {
                card_sum += t;
                qdc_sum += q;
                card_hits++;
                if (t < min_time) min_time = t;
            }
        }

        if (t0_hits == 4 && card_hits > 0) {
            double t0_avg = t0_sum / 4.0;
            double card_avg = card_sum / card_hits;
            double tof = card_avg - t0_avg;
            double tof_min = min_time - t0_avg;

            h_tof->Fill(tof);
            h_qdc->Fill(qdc_sum);
            h_qdc_vs_tof->Fill(tof, qdc_sum);
            h_tof_min->Fill(tof_min);
            h_qdc_min->Fill(qdc_sum);
            h_qdc_vs_tof_min->Fill(tof_min, qdc_sum);

            if (pid_names.count(pid_code)) {
                h_tof_pid[pid_code]->Fill(tof);
                h_qdc_pid[pid_code]->Fill(qdc_sum);
                h_qdc_vs_tof_pid[pid_code]->Fill(tof, qdc_sum);
                h_tof_pid_min[pid_code]->Fill(tof_min);
                h_qdc_pid_min[pid_code]->Fill(qdc_sum);
                h_qdc_vs_tof_pid_min[pid_code]->Fill(tof_min, qdc_sum);
            }
        }
    }

    TCanvas* c = new TCanvas("c", "Plots", 800, 600);
    c->Print(output_pdf + "(");

    c->Clear(); TText* t = new TText(0.5, 0.6, Form("ToF and QDC Analysis (Card %d)", selected_card));
    t->SetTextAlign(22); t->SetTextSize(0.04); t->Draw();
    c->Print(output_pdf);

    for (int i = 0; i < 4; ++i) { c->Clear(); h_t0_ch[i]->Draw(); c->Print(output_pdf); }
    c->Clear(); h_selected_all->Draw(); c->Print(output_pdf);

    // Overlay for average ToF
    c->Clear(); h_tof->SetLineColor(kBlack); h_tof->Draw("hist");
    TLegend* leg1 = new TLegend(0.65, 0.7, 0.88, 0.88);
    leg1->AddEntry(h_tof, "All", "l");
    for (const auto& [pid, h] : h_tof_pid) { h->Draw("hist same"); leg1->AddEntry(h, pid_names.at(pid), "l"); }
    leg1->Draw(); c->Print(output_pdf);

    // Overlay for average QDC
    c->Clear(); h_qdc->SetLineColor(kBlack); h_qdc->Draw("hist");
    TLegend* leg2 = new TLegend(0.65, 0.7, 0.88, 0.88);
    leg2->AddEntry(h_qdc, "All", "l");
    for (const auto& [pid, h] : h_qdc_pid) { h->Draw("hist same"); leg2->AddEntry(h, pid_names.at(pid), "l"); }
    leg2->Draw(); c->Print(output_pdf);

    c->Clear(); h_qdc_vs_tof->Draw("colz"); c->Print(output_pdf);
    for (const auto& [pid, h2d] : h_qdc_vs_tof_pid) {
        c->Clear(); h2d->Draw("colz"); c->Print(output_pdf);
    }

    // Overlay for minimum-time ToF
    c->Clear(); h_tof_min->SetLineColor(kBlack); h_tof_min->Draw("hist");
    TLegend* leg3 = new TLegend(0.65, 0.7, 0.88, 0.88);
    leg3->AddEntry(h_tof_min, "All", "l");
    for (const auto& [pid, h] : h_tof_pid_min) { h->Draw("hist same"); leg3->AddEntry(h, pid_names.at(pid), "l"); }
    leg3->Draw(); c->Print(output_pdf);

    // Overlay for minimum-time QDC
    c->Clear(); h_qdc_min->SetLineColor(kBlack); h_qdc_min->Draw("hist");
    TLegend* leg4 = new TLegend(0.65, 0.7, 0.88, 0.88);
    leg4->AddEntry(h_qdc_min, "All", "l");
    for (const auto& [pid, h] : h_qdc_pid_min) { h->Draw("hist same"); leg4->AddEntry(h, pid_names.at(pid), "l"); }
    leg4->Draw(); c->Print(output_pdf);

    // ✅ NEW plot added here
    c->Clear(); h_qdc_vs_tof_min->Draw("colz"); c->Print(output_pdf);

    for (const auto& [pid, h2d] : h_qdc_vs_tof_pid_min) {
        c->Clear(); h2d->Draw("colz"); c->Print(output_pdf);
    }

    c->Print(output_pdf + ")");
    file->Close();
    return 0;
}
