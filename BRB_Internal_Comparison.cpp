#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TText.h>
#include <TStyle.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <filesystem> // for filename extraction

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <BRB root file>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string filename = std::filesystem::path(filepath).filename().string(); // Only filename

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

    // Branches
    std::vector<float>* brb_qdc = nullptr;
    std::vector<int>* brb_qdc_ids = nullptr;
    std::vector<float>* brb_tdc = nullptr;
    std::vector<int>* brb_tdc_ids = nullptr;
    std::vector<float>* hit_qdc = nullptr;
    std::vector<double>* hit_tdc = nullptr;
    std::vector<int>* hit_card = nullptr;
    std::vector<int>* hit_chan = nullptr;

    treeBRB->SetBranchAddress("beamline_pmt_qdc_charges", &brb_qdc);
    treeBRB->SetBranchAddress("beamline_pmt_qdc_ids", &brb_qdc_ids);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_times", &brb_tdc);
    treeBRB->SetBranchAddress("beamline_pmt_tdc_ids", &brb_tdc_ids);
    treeBRB->SetBranchAddress("hit_pmt_charges", &hit_qdc);
    treeBRB->SetBranchAddress("hit_pmt_times", &hit_tdc);
    treeBRB->SetBranchAddress("hit_mpmt_card_ids", &hit_card);
    treeBRB->SetBranchAddress("hit_pmt_channel_ids", &hit_chan);

    // Read Mapping File
    std::ifstream mappingFile("detector_mapping.txt");
    if (!mappingFile.is_open()) {
        std::cerr << "Error opening detector_mapping.txt!" << std::endl;
        return 1;
    }

    std::map<std::pair<int, int>, int> cardchan_to_beamlineidx;
    std::map<int, std::string> beamlineidx_to_name;
    std::string line;
    std::getline(mappingFile, line); // skip header

    while (std::getline(mappingFile, line)) {
        std::istringstream iss(line);
        std::string name;
        int card, chan, idx;
        char comma;
        std::getline(iss, name, ',');
        iss >> card >> comma >> chan >> comma >> idx;
        cardchan_to_beamlineidx[{card, chan}] = idx;
        beamlineidx_to_name[idx] = name;
    }
    mappingFile.close();

    Long64_t nEntriesBRB = treeBRB->GetEntries();
    nEntriesBRB = 5000; // limit for testing

    std::vector<TH1D*> hists_brb_qdc(64, nullptr);
    std::vector<TH1D*> hists_brb_tdc(64, nullptr);
    std::vector<TH1D*> hists_hit_qdc(64, nullptr);
    std::vector<TH1D*> hists_hit_tdc(64, nullptr);

    for (int i = 0; i < 64; ++i) {
        hists_brb_qdc[i] = new TH1D(Form("hBRB_qdc_%d", i), Form("Beamline QDC ID %d", i), 450, 0, 4500);
        hists_brb_tdc[i] = new TH1D(Form("hBRB_tdc_%d", i), Form("Beamline TDC ID %d", i), 800, 0, 800);
        hists_hit_qdc[i] = new TH1D(Form("hHIT_qdc_%d", i), Form("HitPMT QDC ID %d", i), 850, 0, 8500);
        hists_hit_tdc[i] = new TH1D(Form("hHIT_tdc_%d", i), Form("HitPMT TDC ID %d", i), 8000, 0, 8000);
    }

    for (Long64_t i = 0; i < nEntriesBRB; ++i) {
        treeBRB->GetEntry(i);

        if (brb_qdc && brb_qdc_ids) {
            for (size_t j = 0; j < brb_qdc_ids->size(); ++j) {
                int idx = (*brb_qdc_ids)[j];
                if (idx >= 0 && idx < 64) hists_brb_qdc[idx]->Fill((*brb_qdc)[j]);
            }
        }

        if (brb_tdc && brb_tdc_ids) {
            for (size_t j = 0; j < brb_tdc_ids->size(); ++j) {
                int idx = (*brb_tdc_ids)[j];
                if (idx >= 0 && idx < 64) hists_brb_tdc[idx]->Fill((*brb_tdc)[j]);
            }
        }

        if (hit_card && hit_chan && hit_qdc && hit_tdc) {
            for (size_t j = 0; j < hit_card->size(); ++j) {
                int card = (*hit_card)[j];
                int chan = (*hit_chan)[j];
                auto it = cardchan_to_beamlineidx.find({card, chan});
                if (it != cardchan_to_beamlineidx.end()) {
                    int idx = it->second;
                    if (idx >= 0 && idx < 64) {
                        double tdc_value = (*hit_tdc)[j];
                        if (tdc_value >= 2100 && tdc_value <= 2300) {
                            hists_hit_qdc[idx]->Fill((*hit_qdc)[j]);
                        }
                        hists_hit_tdc[idx]->Fill(tdc_value);
                    }
                }
            }
        }
    }

    TCanvas* c = new TCanvas("c", "Comparison", 1000, 1200);
    c->Print("BRB_Internal_Comparison.pdf(");

    // Title Page
    c->Clear();
    TText* title = new TText(0.5, 0.7, "BRB Internal Comparison");
    title->SetTextAlign(22);
    title->SetTextSize(0.05);
    title->Draw();

    TText* filetxt = new TText(0.5, 0.5, filename.c_str());
    filetxt->SetTextAlign(22);
    filetxt->SetTextSize(0.03);
    filetxt->Draw();
    c->Print("BRB_Internal_Comparison.pdf");

    for (int i = 0; i < 64; ++i) {
        std::string det_name = beamlineidx_to_name.count(i) ? beamlineidx_to_name[i] : Form("ID %d", i);

        // QDC page
        c->Clear();
        c->Divide(1,2);

        TPad* padTop = (TPad*)c->cd(0);
        padTop->SetPad(0,0,1,1);
        TText* pageTitleQDC = new TText(0.5, 0.96, (det_name + " - QDC").c_str());
        pageTitleQDC->SetTextAlign(22);
        pageTitleQDC->SetTextSize(0.04);
        pageTitleQDC->Draw();

        c->cd(1);
        gPad->SetPad(0,0,1,0.5);  
        hists_brb_qdc[i]->Draw();

        c->cd(2);
        gPad->SetPad(0,0.5,1,1); 
        hists_hit_qdc[i]->Draw();

        c->Print("BRB_Internal_Comparison.pdf");

        // TDC page
        c->Clear();
        c->Divide(1,2);

        padTop = (TPad*)c->cd(0);
        padTop->SetPad(0,0,1,1);
        TText* pageTitleTDC = new TText(0.5, 0.96, (det_name + " - TDC").c_str());
        pageTitleTDC->SetTextAlign(22);
        pageTitleTDC->SetTextSize(0.04);
        pageTitleTDC->Draw();

        c->cd(1);
        gPad->SetPad(0,0,1,0.5);
        hists_brb_tdc[i]->Draw();

        c->cd(2);
        gPad->SetPad(0,0.5,1,1);
        hists_hit_tdc[i]->Draw();

        c->Print("BRB_Internal_Comparison.pdf");
    }

    c->Print("BRB_Internal_Comparison.pdf)");

    fileBRB->Close();
    return 0;
}
