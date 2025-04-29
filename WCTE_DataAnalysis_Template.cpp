// WCTE_DataAnalysis_Template.cpp

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TText.h>
#include <TSystem.h>
#include <TString.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "WCTE_BeamMon_PID.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <BRB ROOT file> <boxcuts.json>" << std::endl;
        return 1;
    }

    std::string output_pdf = "pid_selection_plots.pdf";

    std::string filename = argv[1];
    std::string boxcutfile = argv[2];
    TFile* fileBRB = TFile::Open(filename.c_str());
    if (!fileBRB || fileBRB->IsZombie()) {
        std::cerr << "Error opening BRB file!" << std::endl;
        return 1;
    }

    TTree* tree = (TTree*)fileBRB->Get("WCTEReadoutWindows");
    if (!tree) {
        std::cerr << "Tree 'WCTEReadoutWindows' not found!" << std::endl;
        return 1;
    }

    std::vector<float>* brb_qdc = nullptr;
    std::vector<int>*   brb_qdc_ids = nullptr;
    std::vector<float>* brb_tdc = nullptr;
    std::vector<int>*   brb_tdc_ids = nullptr;

    tree->SetBranchAddress("beamline_pmt_qdc_charges", &brb_qdc);
    tree->SetBranchAddress("beamline_pmt_qdc_ids", &brb_qdc_ids);
    tree->SetBranchAddress("beamline_pmt_tdc_times", &brb_tdc);
    tree->SetBranchAddress("beamline_pmt_tdc_ids", &brb_tdc_ids);

    WCTE_BeamMon_PID pid;
    if (!pid.LoadBoxCuts(boxcutfile)) {
        std::cerr << "Failed to load boxcuts from file." << std::endl;
        return 1;
    }

    std::string fname = gSystem->BaseName(filename.c_str());
    size_t pos1 = fname.find("R");
    size_t pos2 = fname.find("S");
    int run_id = 0;
    if (pos1 != std::string::npos && pos2 != std::string::npos && pos2 > pos1) {
        run_id = std::stoi(fname.substr(pos1+1, pos2-pos1-1));
    } else {
        std::cerr << "Cannot extract run number from filename!" << std::endl;
        return 1;
    }
    pid.SetRunID(run_id);

    TH2D* h_all_tof_vs_act = new TH2D("h_all_tof_vs_act", "ACT3-5 vs TOF (All);T1-T0 (ns);ACT3-5 QDC Sum", 100, 10, 20, 500, 0, 20000);
    TH1D* h_all_tof = new TH1D("h_all_tof", "TOF (All);T1-T0 (ns);Counts", 100, 10, 20);
    TH1D* h_all_act = new TH1D("h_all_act", "ACT3-5 (All);ACT3-5 QDC Sum;Counts", 500, 0, 20000);

    const char* types[] = {"Electron", "Muon", "Pion"};
    Color_t colors[] = {kBlue, kRed, kGreen+2};

    TH2D* h_pid_tof_vs_act[3];
    TH1D* h_pid_tof[3];
    TH1D* h_pid_act[3];

    for (int i = 0; i < 3; ++i) {
        h_pid_tof_vs_act[i] = new TH2D(Form("h_%s_tof_vs_act", types[i]), "", 100, 10, 20, 500, 0, 20000);
        h_pid_tof[i] = new TH1D(Form("h_%s_tof", types[i]), "", 100, 10, 20);
        h_pid_act[i] = new TH1D(Form("h_%s_act", types[i]), "", 500, 0, 20000);
    }

    Long64_t nEntries = std::min(tree->GetEntries(), (Long64_t)5000);
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        pid.SetBeamlineData(brb_qdc, brb_qdc_ids, brb_tdc, brb_tdc_ids);

        double tof = pid.GetTofT0T1();
        double act = pid.GetActGroup2Sum();

        if (tof < -90 || act < 0) continue;

        h_all_tof_vs_act->Fill(tof, act);
        h_all_tof->Fill(tof);
        h_all_act->Fill(act);

        int pid_code = pid.GetParticleIDBox();
        if (pid_code == 11) { h_pid_tof_vs_act[0]->Fill(tof, act); h_pid_tof[0]->Fill(tof); h_pid_act[0]->Fill(act); }
        else if (pid_code == 13) { h_pid_tof_vs_act[1]->Fill(tof, act); h_pid_tof[1]->Fill(tof); h_pid_act[1]->Fill(act); }
        else if (pid_code == 211) { h_pid_tof_vs_act[2]->Fill(tof, act); h_pid_tof[2]->Fill(tof); h_pid_act[2]->Fill(act); }
    }

    TCanvas* c = new TCanvas("c", "PID Plots", 1200, 800);
    c->Print((output_pdf + "(").c_str()); // Open file

    // Title page
    c->Clear();
    TText* title = new TText(0.5, 0.7, "Event Selection Plots");
    title->SetTextAlign(22);
    title->SetTextSize(0.04);
    title->Draw();

    TString base_filename = gSystem->BaseName(filename.c_str());
    TText* fname_text = new TText(0.5, 0.4, Form("Input File: %s", base_filename.Data()));
    fname_text->SetTextAlign(22);
    fname_text->SetTextSize(0.03);
    fname_text->Draw();

    c->Print(output_pdf.c_str()); // Save the title page

    c->Clear();
    h_all_tof_vs_act->Draw("colz");
    c->Print(output_pdf.c_str());

    c->Clear();
    TGraph* graph_all = new TGraph(h_all_tof_vs_act->GetEntries());
    TGraph* graph_pid[3];
    for (int i = 0; i < 3; ++i) {
        graph_pid[i] = new TGraph(h_pid_tof_vs_act[i]->GetEntries());
    }

    int idx_all = 0;
    for (int ix = 1; ix <= h_all_tof_vs_act->GetNbinsX(); ++ix) {
        for (int iy = 1; iy <= h_all_tof_vs_act->GetNbinsY(); ++iy) {
            int entries = (int)h_all_tof_vs_act->GetBinContent(ix, iy);
            double x = h_all_tof_vs_act->GetXaxis()->GetBinCenter(ix);
            double y = h_all_tof_vs_act->GetYaxis()->GetBinCenter(iy);
            for (int e = 0; e < entries; ++e) graph_all->SetPoint(idx_all++, x, y);
        }
    }

    for (int p = 0; p < 3; ++p) {
        int idx = 0;
        for (int ix = 1; ix <= h_pid_tof_vs_act[p]->GetNbinsX(); ++ix) {
            for (int iy = 1; iy <= h_pid_tof_vs_act[p]->GetNbinsY(); ++iy) {
                int entries = (int)h_pid_tof_vs_act[p]->GetBinContent(ix, iy);
                double x = h_pid_tof_vs_act[p]->GetXaxis()->GetBinCenter(ix);
                double y = h_pid_tof_vs_act[p]->GetYaxis()->GetBinCenter(iy);
                for (int e = 0; e < entries; ++e) graph_pid[p]->SetPoint(idx++, x, y);
            }
        }
    }

    c->Clear();
    graph_all->SetMarkerStyle(20);
    graph_all->SetMarkerColor(kBlack);
    graph_all->GetXaxis()->SetLimits(10, 20);
    graph_all->GetYaxis()->SetRangeUser(0, 20000);
    graph_all->Draw("AP");

    for (int i = 0; i < 3; ++i) {
        graph_pid[i]->SetMarkerStyle(24);
        graph_pid[i]->SetMarkerColor(colors[i]);
        graph_pid[i]->Draw("P SAME");
    }

    TLegend* leg = new TLegend(0.65, 0.7, 0.88, 0.88);
    leg->AddEntry(graph_all, "All", "p");
    for (int i = 0; i < 3; ++i) {
        leg->AddEntry(graph_pid[i], types[i], "p");
    }
    leg->Draw();
    c->Print(output_pdf.c_str());

    c->Clear();
    c->Divide(1,2);

    c->cd(1);
    h_all_tof->SetLineColor(kBlack);
    h_all_tof->Draw("hist");
    for (int i = 0; i < 3; ++i) {
        h_pid_tof[i]->SetLineColor(colors[i]);
        h_pid_tof[i]->Draw("hist same");
    }

    c->cd(2);
    h_all_act->SetLineColor(kBlack);
    h_all_act->Draw("hist");
    for (int i = 0; i < 3; ++i) {
        h_pid_act[i]->SetLineColor(colors[i]);
        h_pid_act[i]->Draw("hist same");
    }

    c->Print((output_pdf + ")").c_str());

    fileBRB->Close();
    return 0;
}
