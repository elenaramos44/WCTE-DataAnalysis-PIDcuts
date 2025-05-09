#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TSystem.h>
#include <TH2D.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <map>
#include "WCTE_BeamMon_PID.h"
#include "WCTE_DataQuality.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <BRB ROOT file> <boxcuts.json> <PDG code>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string boxcutfile = argv[2];
    int target_pdg = std::stoi(argv[3]);

    TFile* infile = TFile::Open(filename.c_str());
    if (!infile || infile->IsZombie()) {
        std::cerr << "Error opening input ROOT file!" << std::endl;
        return 1;
    }

    TTree* intree = (TTree*)infile->Get("WCTEReadoutWindows");
    if (!intree) {
        std::cerr << "Input tree 'WCTEReadoutWindows' not found!" << std::endl;
        return 1;
    }

    // Read run_id from branch
    int run_id = 0;
    intree->SetBranchAddress("run_id", &run_id);
    intree->GetEntry(0); // Read the first event to initialize run_id

    std::map<int, std::string> pdg_names = {
        {11, "Electron"}, {-11, "Positron"},
        {13, "Muon"},     {-13, "AntiMuon"},
        {211, "Pion+"},   {-211, "Pion-"}
    };

    std::cout << "Run " << run_id << " is being processed.\n";
    std::cout << "Selected particle: PDG " << target_pdg;
    if (pdg_names.count(target_pdg)) std::cout << " (" << pdg_names[target_pdg] << ")";
    std::cout << "\n";

    std::string base = gSystem->BaseName(filename.c_str());
    std::string outname = base.substr(0, base.find(".root")) + Form("_%d.root", target_pdg);

    TFile* outfile = new TFile(outname.c_str(), "RECREATE");
    TTree* outtree = intree->CloneTree(0);

    int pdg_value;
    TBranch* pdg_branch = outtree->Branch("PDG", &pdg_value, "PDG/I");

    // Add histograms
    TH2D* h_all = new TH2D("h_all_tof_vs_act", "ACT vs TOF (All);ToF (ns);ACT3-5 QDC", 100, 10, 20, 500, 0, 20000);
    TH2D* h_sel = new TH2D("h_sel_tof_vs_act", "ACT vs TOF (Selected);ToF (ns);ACT3-5 QDC", 100, 10, 20, 500, 0, 20000);

    // Set branches for PID
    std::vector<float>* qdc = nullptr;
    std::vector<int>* qdc_ids = nullptr;
    std::vector<float>* tdc = nullptr;
    std::vector<int>* tdc_ids = nullptr;

    intree->SetBranchAddress("beamline_pmt_qdc_charges", &qdc);
    intree->SetBranchAddress("beamline_pmt_qdc_ids", &qdc_ids);
    intree->SetBranchAddress("beamline_pmt_tdc_times", &tdc);
    intree->SetBranchAddress("beamline_pmt_tdc_ids", &tdc_ids);

    WCTE_BeamMon_PID pid;
    pid.LoadBoxCuts(boxcutfile);
    pid.SetRunID(run_id);
    pid.SetPIDMethod("box");

    Long64_t nentries = intree->GetEntries();
    std::cout << "Total number of events: " << nentries << "\n";
    int selected_count = 0;

    for (Long64_t i = 0; i < nentries; ++i) {
        intree->GetEntry(i);
        pid.SetBeamlineData(qdc, qdc_ids, tdc, tdc_ids);

        double tof = pid.GetTofT0T1();
        double act = pid.GetActGroup2Sum();
        h_all->Fill(tof, act);

        int pid_code = pid.GetParticleID();
        if (pid_code == std::abs(target_pdg)) {
            pdg_value = pid_code;
            outtree->Fill();
            h_sel->Fill(tof, act);
            selected_count++;
        }
    }

    std::cout << "Number of selected events: " << selected_count << "\n";

    outfile->cd();
    outtree->Write();
    h_all->Write();
    h_sel->Write();
    outfile->Close();
    infile->Close();
    return 0;
}
