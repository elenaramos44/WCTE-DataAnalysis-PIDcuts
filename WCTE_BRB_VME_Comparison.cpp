#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TText.h>
#include <TSystem.h>
#include <TString.h>
#include <iostream>
#include <vector>

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
    std::vector<double>* vme_qdc = nullptr;
    std::vector<std::vector<double>>* vme_tdc = nullptr;
    std::vector<std::string>* vme_id_names = nullptr;

    treeBRB->SetBranchAddress("beamline_pmt_qdc_charges", &brb_qdc);
    treeBRB->SetBranchAddress("beamline_pmt_qdc_ids", &brb_qdc_ids);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_times", &brb_tdc);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_ids", &brb_tdc_ids);

    treeVME->SetBranchAddress("beamline_qdc_charge", &vme_qdc);
    treeVME->SetBranchAddress("beamline_tdc_time", &vme_tdc);
    treeVME->SetBranchAddress("beamline_id_name", &vme_id_names);

    const int nChannels = 64;

    TH1D* hBRB_QDC_All = new TH1D("hBRB_QDC_All", "BRB QDC All Channels;QDC;Counts", 410, 0, 4500);
    TH1D* hVME_QDC_All = new TH1D("hVME_QDC_All", "VME QDC All Channels;QDC;Counts", 410, 0, 4500);
    TH1D* hBRB_TDC_All = new TH1D("hBRB_TDC_All", "BRB TDC All Channels;TDC (ns);Counts", 400, -200, 600);
    TH1D* hVME_TDC_All = new TH1D("hVME_TDC_All", "VME TDC All Channels;TDC (ns);Counts", 400, -200, 600);

    hBRB_QDC_All->SetDirectory(0);
    hVME_QDC_All->SetDirectory(0);
    hBRB_TDC_All->SetDirectory(0);
    hVME_TDC_All->SetDirectory(0);

    std::vector<TH1D*> hists_qdc;
    std::vector<TH1D*> hists_tdc;

    for (int ch = 0; ch < nChannels; ++ch) {
        auto h1 = new TH1D(Form("hBRB_qdc_ch%d", ch), Form("BRB QDC Channel %d;QDC;Counts", ch), 410, 0, 4500);
        auto h2 = new TH1D(Form("hVME_qdc_ch%d", ch), Form("VME QDC Channel %d;QDC;Counts", ch), 410, 0, 4500);
        auto h3 = new TH1D(Form("hBRB_tdc_ch%d", ch), Form("BRB TDC Channel %d;TDC (ns);Counts", ch), 400, -200, 600);
        auto h4 = new TH1D(Form("hVME_tdc_ch%d", ch), Form("VME TDC Channel %d;TDC (ns);Counts", ch), 400, -200, 600);

        h1->SetDirectory(0);
        h2->SetDirectory(0);
        h3->SetDirectory(0);
        h4->SetDirectory(0);

        hists_qdc.push_back(h1);
        hists_qdc.push_back(h2);
        hists_tdc.push_back(h3);
        hists_tdc.push_back(h4);
    }

    Long64_t nEntriesBRB = treeBRB->GetEntries();
    nEntriesBRB = 5000;
    for (Long64_t i = 0; i < nEntriesBRB; ++i) {
        treeBRB->GetEntry(i);
        if (brb_qdc && brb_qdc_ids) {
            for (size_t idx = 0; idx < brb_qdc_ids->size(); ++idx) {
                int ch = (*brb_qdc_ids)[idx];
                if (ch >= 0 && ch < nChannels) {
                    hists_qdc[ch*2]->Fill((*brb_qdc)[idx]);
                    hBRB_QDC_All->Fill((*brb_qdc)[idx]);
                }
            }
        }
        if (brb_tdc && brb_tdc_ids) {
            for (size_t idx = 0; idx < brb_tdc_ids->size(); ++idx) {
                int ch = (*brb_tdc_ids)[idx];
                if (ch >= 0 && ch < nChannels) {
                    hists_tdc[ch*2]->Fill((*brb_tdc)[idx]);
                    hBRB_TDC_All->Fill((*brb_tdc)[idx]);
                }
            }
        }
    }

    Long64_t nEntriesVME = treeVME->GetEntries();
    nEntriesVME = 5000;
    for (Long64_t i = 0; i < nEntriesVME; ++i) {
        treeVME->GetEntry(i);
        if (vme_qdc) {
            for (int ch = 0; ch < nChannels; ++ch) {
                if (ch < vme_qdc->size()) {
                    hists_qdc[ch*2+1]->Fill((*vme_qdc)[ch]);
                    hVME_QDC_All->Fill((*vme_qdc)[ch]);
                }
            }
        }
        if (vme_tdc) {
            for (int ch = 0; ch < nChannels; ++ch) {
                if (ch < vme_tdc->size()) {
                    for (double val : (*vme_tdc)[ch]) {
                        hists_tdc[ch*2+1]->Fill(val + 250.0);
                        hVME_TDC_All->Fill(val + 250.0);
                    }
                }
            }
        }
    }

    std::vector<std::string> id_names;
    treeVME->GetEntry(0);
    if (vme_id_names) {
        for (auto& name : *vme_id_names) id_names.push_back(name);
    }

    TCanvas* cTitle = new TCanvas("cTitle", "Title Page", 800, 600);
    cTitle->Print("comparison_report.pdf(");
    cTitle->cd();
    TText* title = new TText(0.5, 0.8, "Comparison Report");
    title->SetTextAlign(22);
    title->SetTextSize(0.04);
    title->Draw();

    TString file1(argv[1]);
    TString file2(argv[2]);
    file1 = gSystem->BaseName(file1);
    file2 = gSystem->BaseName(file2);

    TText* t1 = new TText(0.5, 0.5, Form("BRB File: %s", file1.Data()));
    t1->SetTextAlign(22);
    t1->SetTextSize(0.03);
    t1->Draw();

    TText* t2 = new TText(0.5, 0.4, Form("VME File: %s", file2.Data()));
    t2->SetTextAlign(22);
    t2->SetTextSize(0.03);
    t2->Draw();

    cTitle->Print("comparison_report.pdf");

    TCanvas* cSec1 = new TCanvas("cSec1", "Section 1", 800, 600);
    TText* sec1 = new TText(0.5, 0.5, "Plots of all vector elements in one histogram for QDC and TDC");
    sec1->SetTextAlign(22);
    sec1->SetTextSize(0.03);
    sec1->Draw();
    cSec1->Print("comparison_report.pdf");

    TCanvas* cAll = new TCanvas("cAll", "All Channels", 1200, 800);
    cAll->Divide(2,2);
    cAll->cd(1); hBRB_QDC_All->Draw();
    cAll->cd(2); hVME_QDC_All->Draw();
    cAll->cd(3); hBRB_TDC_All->Draw();
    cAll->cd(4); hVME_TDC_All->Draw();
    cAll->Print("comparison_report.pdf");

    TCanvas* cSec2 = new TCanvas("cSec2", "Section 2", 800, 600);
    TText* sec2 = new TText(0.5, 0.5, "Plots for each vector element / detector");
    sec2->SetTextAlign(22);
    sec2->SetTextSize(0.03);
    sec2->Draw();
    cSec2->Print("comparison_report.pdf");

    for (int ch = 0; ch < nChannels; ++ch) {
        TCanvas* c = new TCanvas(Form("cCh%d", ch), Form("Channel %d", ch), 1000, 800);
        c->Divide(1,2);
        c->cd(1); hists_qdc[ch*2]->Draw();
        c->cd(2); hists_qdc[ch*2+1]->Draw();

        c->cd();
        TText* label = new TText(0.5, 0.95, Form("Channel %d: %s", ch, (ch<id_names.size()?id_names[ch].c_str():"Unknown")));
        label->SetTextAlign(22);
        label->SetTextSize(0.03);
        label->Draw("same");

        c->Print("comparison_report.pdf");

        c->cd(1); hists_tdc[ch*2]->Draw();
        c->cd(2); hists_tdc[ch*2+1]->Draw();

        c->cd();
        TText* label2 = new TText(0.5, 0.95, Form("Channel %d: %s", ch, (ch<id_names.size()?id_names[ch].c_str():"Unknown")));
        label2->SetTextAlign(22);
        label2->SetTextSize(0.03);
        label2->Draw("same");

        c->Print("comparison_report.pdf");

        delete c;
    }

    TCanvas* cEnd = new TCanvas("cEnd", "End", 800, 600);
    cEnd->Print("comparison_report.pdf)");

    fileBRB->Close();
    fileVME->Close();

    return 0;
}
