// Updated PID Plots Comparison with ana_calib-style event selection
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TText.h>
#include <TSystem.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <cmath>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <BRB root file> <VME root file>" << std::endl;
        return 1;
    }

    TFile* fileBRB = TFile::Open(argv[1]);
    TFile* fileVME = TFile::Open(argv[2]);

    if (!fileBRB || !fileVME || fileBRB->IsZombie() || fileVME->IsZombie()) {
        std::cerr << "Error opening files!" << std::endl;
        return 1;
    }

    TTree* treeBRB = (TTree*)fileBRB->Get("WCTEReadoutWindows");
    TTree* treeVME = (TTree*)fileVME->Get("beam_monitor_calib");

    if (!treeBRB || !treeVME) {
        std::cerr << "Error: Could not find the trees!" << std::endl;
        return 1;
    }

    std::vector<float>* brb_qdc = nullptr;
    std::vector<int>* brb_qdc_ids = nullptr;
    std::vector<float>* brb_tdc = nullptr;
    std::vector<int>* brb_tdc_ids = nullptr;
    treeBRB->SetBranchAddress("beamline_pmt_qdc_charges", &brb_qdc);
    treeBRB->SetBranchAddress("beamline_pmt_qdc_ids", &brb_qdc_ids);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_times", &brb_tdc);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_ids", &brb_tdc_ids);

    std::vector<double>* vme_qdc = nullptr;
    std::vector<std::vector<double>>* vme_tdc = nullptr;
    treeVME->SetBranchAddress("beamline_qdc_charge", &vme_qdc);
    treeVME->SetBranchAddress("beamline_tdc_time", &vme_tdc);

    const int nChannels = 64;
    Long64_t nEntriesBRB = std::min(treeBRB->GetEntries(), (Long64_t)5000);
    Long64_t nEntriesVME = std::min(treeVME->GetEntries(), (Long64_t)5000);

    TH1D* h_brb_tof_t0t1 = new TH1D("h_brb_tof_t0t1", "BRB TOF T1-T0;T1-T0 (ns);Counts", 100, 10, 20);
    TH1D* h_vme_tof_t0t1 = new TH1D("h_vme_tof_t0t1", "VME TOF T1-T0;T1-T0 (ns);Counts", 100, 10, 20);
    TH1D* h_brb_act_group2_sum = new TH1D("h_brb_act_group2_sum", "BRB ACT3-5 Sum;Charge;Counts", 900, 0, 18000);
    TH1D* h_vme_act_group2_sum = new TH1D("h_vme_act_group2_sum", "VME ACT3-5 Sum;Charge;Counts", 900, 0, 18000);
    TH2D* h_brb_act_group2_sum_tof_t0t1 = new TH2D("h_brb_act_group2_sum_tof_t0t1", "BRB ACT3-5 Sum vs TOF;T1-T0 (ns);Charge", 100, 10, 20, 900, 0, 18000);
    TH2D* h_vme_act_group2_sum_tof_t0t1 = new TH2D("h_vme_act_group2_sum_tof_t0t1", "VME ACT3-5 Sum vs TOF;T1-T0 (ns);Charge", 100, 10, 20, 900, 0, 18000);

    for (Long64_t i = 0; i < nEntriesBRB; ++i) {
        treeBRB->GetEntry(i);

        double t0 = 0, t1 = 0;
        int t0hits = 0, t1hits = 0;
        bool t4_hit = false;
        bool hole0 = false, hole1 = false;
        double act_sum = 0;

        for (size_t j = 0; brb_qdc_ids && j < brb_qdc_ids->size(); ++j) {
            int ch = (*brb_qdc_ids)[j];
            float qdc = (*brb_qdc)[j];
            if (ch == 42 || ch == 43) if (qdc > 300) t4_hit = true;
            if (ch == 9 && qdc > 150) hole0 = true;
            if (ch == 10 && qdc > 100) hole1 = true;
            if (ch >= 18 && ch <= 23) act_sum += qdc;
        }

        if (!t4_hit || hole0 || hole1) continue;

        for (size_t j = 0; brb_tdc_ids && j < brb_tdc_ids->size(); ++j) {
            int ch = (*brb_tdc_ids)[j];
            float tdc = (*brb_tdc)[j] - 250;
            if (ch >= 0 && ch <= 3 && tdc < -100) { t0 += tdc; ++t0hits; }
            if (ch >= 4 && ch <= 7 && tdc < -100) { t1 += tdc; ++t1hits; }
        }

        if (t0hits == 4 && t1hits == 4) {
            double tof = (t1 / 4.0) - (t0 / 4.0);
            h_brb_tof_t0t1->Fill(tof);
            h_brb_act_group2_sum->Fill(act_sum);
            h_brb_act_group2_sum_tof_t0t1->Fill(tof, act_sum);
        }
    }

    for (Long64_t i = 0; i < nEntriesVME; ++i) {
        treeVME->GetEntry(i);

        double t0 = 0, t1 = 0;
        int t0hits = 0, t1hits = 0;
        double act_sum = 0;

        for (int ch = 0; ch < 64 && vme_tdc && ch < vme_tdc->size(); ++ch) {
            for (double val : (*vme_tdc)[ch]) {
                if (ch >= 0 && ch <= 3 && val < -100) { t0 += val; ++t0hits; }
                if (ch >= 4 && ch <= 7 && val < -100) { t1 += val; ++t1hits; }
            }
        }

        if (t0hits == 4 && t1hits == 4) {
            double tof = (t1 / 4.0) - (t0 / 4.0);
            h_vme_tof_t0t1->Fill(tof);

            if (vme_qdc) {
                for (int ch = 18; ch <= 23 && ch < vme_qdc->size(); ++ch) {
                    act_sum += (*vme_qdc)[ch];
                }
            }
            h_vme_act_group2_sum->Fill(act_sum);
            h_vme_act_group2_sum_tof_t0t1->Fill(tof, act_sum);
        }
    }

    TCanvas* c = new TCanvas("cPID", "PID Comparison", 1200, 800);
    c->Print("comparison_report.pdf(");
    TText* title = new TText(0.5, 0.5, "PID Plots Comparison");
    title->SetTextAlign(22);
    title->SetTextSize(0.04);
    title->Draw();
    c->Print("EvSelPlots_comparison_report.pdf");

    std::vector<std::pair<TH1*, TH1*>> oneD = {
        {h_brb_tof_t0t1, h_vme_tof_t0t1},
        {h_brb_act_group2_sum, h_vme_act_group2_sum}
    };

    for (auto& [h1, h2] : oneD) {
        c->Clear(); c->Divide(1,2);
        c->cd(1); h1->Draw();
        c->cd(2); h2->Draw();
        c->Print("EvSelPlots_comparison_report.pdf");
    }

    std::vector<std::pair<TH2*, TH2*>> twoD = {
        {h_brb_act_group2_sum_tof_t0t1, h_vme_act_group2_sum_tof_t0t1}
    };

    for (auto& [h1, h2] : twoD) {
        c->Clear(); c->Divide(1,2);
        c->cd(1); h1->Draw("colz");
        c->cd(2); h2->Draw("colz");
        c->Print("EvSelPlots_comparison_report.pdf");
    }

    c->Print("EvSelPlots_comparison_report.pdf)");
    fileBRB->Close(); fileVME->Close();
    return 0;
}
