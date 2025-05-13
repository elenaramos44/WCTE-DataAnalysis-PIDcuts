// This program reads a WCSim ROOT file and writes a flat ROOT file with:
// - Tracks
// - CherenkovDigiHits
// - Trigger
// - WCTEData (fake format matching WCTEReadoutWindows real data tree)

#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TClonesArray.h"
#include "WCSimRootEvent.hh"
#include "WCSimRootGeom.hh"
#include "WCSimRootOptions.hh"
#include "WCSimEnumerations.hh"
#include <vector>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

std::unordered_map<int, std::pair<int, int>> load_tube_mapping(const std::string& filename) {
    std::unordered_map<int, std::pair<int, int>> map;
    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int tube, slot, channel;
        std::string skip;
        int col10 = 0; // default to 0 in case the column is missing

        // Read first 3 fields
        if (!(iss >> tube >> slot >> channel)) continue;

        // Skip columns 4–9
        for (int i = 0; i < 6; ++i) {
            if (!(iss >> skip)) break;
        }

        // Read column 10 (index 9)
        if (!(iss >> col10)) continue;

        // Only insert if column 10 is non-zero
        if (col10 != 0) {
            map[tube] = {slot, channel-1};
        }
    }

    return map;
}

std::unordered_map<int, int> get_slot_to_card_map() {
  return {
    {1,12},{2,27},{3,108},{4,46},{5,117},{6,52},{7,82},{8,96},{10,11},{11,94},
    {13,47},{19,114},{20,101},{21,45},{22,102},{23,77},{24,100},{25,92},{26,113},
    {28,83},{29,17},{30,80},{31,73},{33,78},{34,7},{35,112},{36,79},{37,48},
    {38,105},{39,6},{40,104},{41,19},{43,44},{44,107},{46,36},{47,23},{49,41},
    {50,29},{51,43},{52,30},{53,14},{54,31},{55,118},{56,28},{57,115},{58,15},
    {60,26},{61,10},{62,25},{64,21},{65,38},{66,106},{68,3},{69,18},{70,1},
    {71,24},{72,40},{73,16},{75,32},{76,35},{78,34},{80,42},{81,20},{82,22},
    {83,33},{84,8},{86,76},{87,84},{88,87},{89,89},{90,99},{92,97},{93,85},
    {94,91},{95,75},{97,111},{98,103},{100,98},{101,93},{103,71},{104,109},{105,86}
  };
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input_directory>" << std::endl;
    return 1;
  }


  TDirectory::AddDirectory(kFALSE);

std::string inputDirectory = argv[1];
// Normalize and remove trailing slashes
fs::path inputPath = fs::weakly_canonical(fs::path(inputDirectory));
std::string dirNameOnly = inputPath.filename().string();
if (dirNameOnly.empty()) dirNameOnly = inputPath.parent_path().filename().string();
std::string outputFileName = dirNameOnly + "_masked.root";


  auto tube_map = load_tube_mapping("tube-slot_channel-mapping_v2_modified.txt");
  auto slot_to_card = get_slot_to_card_map();

  TFile* outputFile = new TFile(outputFileName.c_str(), "RECREATE");

  TTree* trackTree = new TTree("Tracks", "Tracks Tree");
  const int maxTracks = 10000;
  int event, subevent = 0, Ntracks;
  int Pid[maxTracks], TrackID[maxTracks], ParentID[maxTracks], ProcessType[maxTracks];
  float mass[maxTracks], momentum[maxTracks], Energy[maxTracks], digitime[maxTracks];
  float Dirx[maxTracks], Diry[maxTracks], Dirz[maxTracks];
  float Start_x[maxTracks], Start_y[maxTracks], Start_z[maxTracks];
  float Stop_x[maxTracks], Stop_y[maxTracks], Stop_z[maxTracks];

  trackTree->Branch("Event", &event, "Event/I");
  trackTree->Branch("SubEvent", &subevent, "SubEvent/I");
  trackTree->Branch("Ntracks", &Ntracks, "Ntracks/I");
  trackTree->Branch("Pid", Pid, "Pid[Ntracks]/I");
  trackTree->Branch("Mass", mass, "Mass[Ntracks]/F");
  trackTree->Branch("P", momentum, "P[Ntracks]/F");
  trackTree->Branch("Energy", Energy, "Energy[Ntracks]/F");
  trackTree->Branch("ParentID", ParentID, "ParentID[Ntracks]/I");
  trackTree->Branch("TrackID", TrackID, "TrackID[Ntracks]/I");
  trackTree->Branch("ProcessType", ProcessType, "ProcessType[Ntracks]/I");
  trackTree->Branch("Time", digitime, "Time[Ntracks]/F");
  trackTree->Branch("Dirx", Dirx, "Dirx[Ntracks]/F");
  trackTree->Branch("Diry", Diry, "Diry[Ntracks]/F");
  trackTree->Branch("Dirz", Dirz, "Dirz[Ntracks]/F");
  trackTree->Branch("Start_x", Start_x, "Start_x[Ntracks]/F");
  trackTree->Branch("Start_y", Start_y, "Start_y[Ntracks]/F");
  trackTree->Branch("Start_z", Start_z, "Start_z[Ntracks]/F");
  trackTree->Branch("Stop_x", Stop_x, "Stop_x[Ntracks]/F");
  trackTree->Branch("Stop_y", Stop_y, "Stop_y[Ntracks]/F");
  trackTree->Branch("Stop_z", Stop_z, "Stop_z[Ntracks]/F");

  TTree* digiTree = new TTree("CherenkovDigiHits", "Cherenkov Digitized Hits Tree");
  TTree* triggerTree = new TTree("Trigger", "Trigger Tree");
  TTree* wcteTree = new TTree("WCTEReadoutWindows", "Fake WCTEReadoutWindows");

  int ndigi;
  float PMTQ[2100], PMTT[2100], PMT_x[2100], PMT_y[2100], PMT_z[2100];
  int tube[2100];
  double startTime;

  digiTree->Branch("Event", &event, "Event/I");
  digiTree->Branch("SubEvent", &subevent, "SubEvent/I");
  digiTree->Branch("NDigiHits", &ndigi, "NDigiHits/I");
  digiTree->Branch("Q", PMTQ, "Q[NDigiHits]/F");
  digiTree->Branch("T", PMTT, "T[NDigiHits]/F");
  digiTree->Branch("Tube", tube, "Tube[NDigiHits]/I");
  digiTree->Branch("PMT_x", PMT_x, "PMT_x[NDigiHits]/F");
  digiTree->Branch("PMT_y", PMT_y, "PMT_y[NDigiHits]/F");
  digiTree->Branch("PMT_z", PMT_z, "PMT_z[NDigiHits]/F");

  triggerTree->Branch("Event", &event, "Event/I");
  triggerTree->Branch("SubEvent", &subevent, "SubEvent/I");
  triggerTree->Branch("StartTime", &startTime, "StartTime/D");

  double window_time = 0;
  Long_t start_counter = 0;
  int run_id = 0, sub_run_id = 0, spill_counter = 0, readout_number = 0;
  std::vector<int> hit_card_ids, hit_channels, hit_slots, hit_positions;
  std::vector<float> hit_q;
  std::vector<double> hit_t;
  std::vector<int> trigger_types, led_ids, led_card_ids, led_slot_numbers, led_event_types, led_types, led_sequence_numbers, led_counters;
  std::vector<float> led_gains, led_dacsettings;
  std::vector<double> trigger_times;
  std::vector<int> pmt_waveform_mpmt_card_ids, pmt_waveform_pmt_channel_ids, pmt_waveform_mpmt_slot_ids, pmt_waveform_pmt_position_ids;
  std::vector<double> pmt_waveform_times;
  std::vector<std::vector<double>> pmt_waveforms;
  std::vector<float> brb_qdc;
  std::vector<int> brb_qdc_ids;
  std::vector<float> brb_tdc;
  std::vector<int> brb_tdc_ids;

  wcteTree->Branch("event_number", &event);
  wcteTree->Branch("window_time", &window_time);
  wcteTree->Branch("start_counter", &start_counter);
  wcteTree->Branch("run_id", &run_id);
  wcteTree->Branch("sub_run_id", &sub_run_id);
  wcteTree->Branch("spill_counter", &spill_counter);
  wcteTree->Branch("readout_number", &readout_number);
  wcteTree->Branch("hit_mpmt_card_ids", &hit_card_ids);
  wcteTree->Branch("hit_pmt_channel_ids", &hit_channels);
  wcteTree->Branch("hit_mpmt_slot_ids", &hit_slots);
  wcteTree->Branch("hit_pmt_position_ids", &hit_positions);
  wcteTree->Branch("hit_pmt_charges", &hit_q);
  wcteTree->Branch("hit_pmt_times", &hit_t);
  wcteTree->Branch("trigger_types", &trigger_types);
  wcteTree->Branch("trigger_times", &trigger_times);
  wcteTree->Branch("led_gains", &led_gains);
  wcteTree->Branch("led_dacsettings", &led_dacsettings);
  wcteTree->Branch("led_ids", &led_ids);
  wcteTree->Branch("led_card_ids", &led_card_ids);
  wcteTree->Branch("led_slot_numbers", &led_slot_numbers);
  wcteTree->Branch("led_event_types", &led_event_types);
  wcteTree->Branch("led_types", &led_types);
  wcteTree->Branch("led_sequence_numbers", &led_sequence_numbers);
  wcteTree->Branch("led_counters", &led_counters);
  wcteTree->Branch("beamline_pmt_qdc_charges", &brb_qdc);
  wcteTree->Branch("beamline_pmt_qdc_ids", &brb_qdc_ids);
  wcteTree->Branch("beamline_pmt_tdc_times", &brb_tdc);
  wcteTree->Branch("beamline_pmt_tdc_ids", &brb_tdc_ids);

  
  for (const auto& entry : fs::directory_iterator(inputDirectory)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".root") continue;

    TFile* inputFile = new TFile(entry.path().c_str(), "READ");
    if (!inputFile || inputFile->IsZombie()) continue;

    TTree* eventTree = (TTree*)inputFile->Get("wcsimT");
    TTree* geoTree = (TTree*)inputFile->Get("wcsimGeoT");
    WCSimRootGeom* geo = nullptr;
    geoTree->SetBranchAddress("wcsimrootgeom", &geo);
    geoTree->GetEntry(0);

    WCSimRootEvent* wcsimEvent = new WCSimRootEvent();
    TBranch* branch = eventTree->GetBranch("wcsimrootevent");
    branch->SetAddress(&wcsimEvent);

    Long64_t nEntries = eventTree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
      eventTree->GetEntry(i);
      auto* trigger = wcsimEvent->GetTrigger(0);

      event = trigger->GetHeader()->GetEvtNum();
      startTime = trigger->GetHeader()->GetDate();

      Ntracks = 0;
      for (int j = 2; j < trigger->GetNtrack_slots() && Ntracks < maxTracks; ++j) {
        auto* t = (WCSimRootTrack*)trigger->GetTracks()->At(j);
        if (!t) continue;
        Pid[Ntracks] = t->GetIpnu();
        mass[Ntracks] = t->GetM();
        momentum[Ntracks] = t->GetP();
        Energy[Ntracks] = t->GetE();
        ParentID[Ntracks] = t->GetParentId();
        TrackID[Ntracks] = t->GetId();
        ProcessType[Ntracks] = t->GetCreatorProcess();
        digitime[Ntracks] = t->GetTime();
        Dirx[Ntracks] = t->GetDir(0);
        Diry[Ntracks] = t->GetDir(1);
        Dirz[Ntracks] = t->GetDir(2);
        Start_x[Ntracks] = t->GetStart(0);
        Start_y[Ntracks] = t->GetStart(1);
        Start_z[Ntracks] = t->GetStart(2);
        Stop_x[Ntracks] = t->GetStop(0);
        Stop_y[Ntracks] = t->GetStop(1);
        Stop_z[Ntracks] = t->GetStop(2);
        ++Ntracks;
      }
      trackTree->Fill();

      ndigi = 0;
      hit_card_ids.clear(); hit_channels.clear(); hit_slots.clear(); hit_positions.clear(); hit_q.clear(); hit_t.clear();

      for (int j = 0; j < std::min(trigger->GetNcherenkovdigihits_slots(), 2100); ++j) {
        auto* d = (WCSimRootCherenkovDigiHit*)trigger->GetCherenkovDigiHits()->At(j);
        if (!d) continue;
        int tid = d->GetTubeId();
        tube[ndigi] = tid;
        PMTQ[ndigi] = d->GetQ();
        PMTT[ndigi] = d->GetT();
        auto* pmt = geo->GetPMTPtr(tid - 1);
        PMT_x[ndigi] = pmt->GetPosition(0);
        PMT_y[ndigi] = pmt->GetPosition(1);
        PMT_z[ndigi] = pmt->GetPosition(2);

        hit_q.push_back(d->GetQ());
        hit_t.push_back(d->GetT());
        if (tube_map.count(tid)) {
          int slot = tube_map[tid].first;
          int channel = tube_map[tid].second;
          int card = slot_to_card.count(slot) ? slot_to_card.at(slot) : -1;
          hit_slots.push_back(slot);
          hit_positions.push_back(channel);
          hit_channels.push_back(channel);
          hit_card_ids.push_back(card);
        }
        ++ndigi;
      }
      digiTree->Fill();
      triggerTree->Fill();

      trigger_types = {0}; trigger_times = {0};
      led_gains = {0}; led_dacsettings = {0}; led_ids = {0}; led_card_ids = {0};
      led_slot_numbers = {0}; led_event_types = {0}; led_types = {0};
      led_sequence_numbers = {0}; led_counters = {0};
      pmt_waveform_mpmt_card_ids = {0}; pmt_waveform_pmt_channel_ids = {0};
      pmt_waveform_mpmt_slot_ids = {0}; pmt_waveform_pmt_position_ids = {0};
      pmt_waveform_times = {0}; pmt_waveforms = {{0}};
      brb_qdc = {0}; brb_qdc_ids = {0}; brb_tdc = {0}; brb_tdc_ids = {0};

      wcteTree->Fill();
      wcsimEvent->ReInitialize();
    }
    inputFile->Close();
    delete inputFile;
  }

  


outputFile->cd();
trackTree->Write("", TObject::kOverwrite);
digiTree->Write("", TObject::kOverwrite);
triggerTree->Write("", TObject::kOverwrite);
wcteTree->Write("", TObject::kOverwrite);
outputFile->Close();

  std::cout << "Finished writing flat file: " << outputFileName << std::endl;
  return 0;
}
