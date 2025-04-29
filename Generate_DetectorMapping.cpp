#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>

int main() {
    // Define the BRB mapping (card/channel → name)
    std::map<std::pair<int, int>, std::string> id_names = {
        {{130, 0},  "ACT0-L"}, {{130, 1},  "ACT0-R"}, {{130, 2},  "ACT1-L"}, {{130, 3},  "ACT1-R"},
        {{130, 4},  "NC"},     {{130, 5},  "ACT2-L"}, {{130, 6},  "ACT2-R"}, {{130, 7},  "ACT3-L"},
        {{130, 8},  "ACT3-R"}, {{130, 9},  "ACT4-L"}, {{130, 10}, "ACT4-R"}, {{130, 11}, "ACT5-L"},
        {{130, 12}, "ACT5-R"}, {{130, 13}, "T1-0L"},  {{130, 14}, "T1-0R"},  {{130, 15}, "T1-1L"},
        {{130, 16}, "T1-1R"},  {{130, 17}, "HC-0"},   {{130, 18}, "HC-1"},   {{130, 19}, "Trigger-130"},
        {{131, 0},  "Trigger-131"}, {{131, 1}, "Lemo-1"}, {{131, 2}, "Lemo-2"}, {{131, 3}, "Lemo-3"},
        {{131, 4},  "Lemo-4"}, {{131, 5}, "Lemo-5"}, {{131, 6}, "NC"}, {{131, 7}, "Lemo-6"},
        {{131, 8},  "NC"},     {{131, 9}, "Laser"},  {{131, 10}, "T2"}, {{131, 11}, "T3"},
        {{131, 12}, "T0-0L"},  {{131, 13}, "T0-0R"}, {{131, 14}, "T0-1L"}, {{131, 15}, "T0-1R"},
        {{131, 16}, "NC"},     {{131, 17}, "PbG"},   {{131, 18}, "MuL"}, {{131, 19}, "MuR"},
        {{132, 0},  "TOF-0"},  {{132, 1}, "TOF-1"},  {{132, 2}, "TOF-2"}, {{132, 3}, "TOF-3"},
        {{132, 4},  "TOF-4"},  {{132, 5}, "TOF-5"},  {{132, 6}, "TOF-6"}, {{132, 7}, "TOF-7"},
        {{132, 8},  "TOF-8"},  {{132, 9}, "NC"},     {{132, 10}, "TOF-9"}, {{132, 11}, "TOF-A"},
        {{132, 12}, "TOF-B"},  {{132, 13}, "TOF-C"}, {{132, 14}, "TOF-D"}, {{132, 15}, "TOF-E"},
        {{132, 16}, "TOF-F"},  {{132, 17}, "T4-L"},  {{132, 18}, "T4-R"}, {{132, 19}, "Trigger-132"}
    };

    // Open the VME file
    TFile* vme_file = TFile::Open("../Beam-Analysis/data/beamline_run1626_tuple_calib.root");
    if (!vme_file || vme_file->IsZombie()) {
        std::cerr << "Error opening VME file!" << std::endl;
        return 1;
    }

    TTree* vme_tree = (TTree*)vme_file->Get("beam_monitor_calib");
    if (!vme_tree) {
        std::cerr << "Error: Could not find 'beam_monitor_calib' tree!" << std::endl;
        return 1;
    }

    std::vector<std::string>* vme_id_names = nullptr;
    vme_tree->SetBranchAddress("beamline_id_name", &vme_id_names);

    // Read just the first entry (id names are static)
    vme_tree->GetEntry(0);

    if (!vme_id_names) {
        std::cerr << "Error: VME id names not loaded!" << std::endl;
        return 1;
    }

    std::cout << "Loaded " << vme_id_names->size() << " beamline_id_name entries" << std::endl;

    // Now generate mapping
    std::ofstream outfile("detector_mapping.txt");
    if (!outfile.is_open()) {
        std::cerr << "Error opening detector_mapping.txt for writing!" << std::endl;
        return 1;
    }

    outfile << "detector_name,card,channel,beamline_index\n";

    for (const auto& [key, name] : id_names) {
        int card = key.first;
        int channel = key.second;

        // Find index in vme_id_names
        int found_index = -1;
        for (size_t i = 0; i < vme_id_names->size(); ++i) {
            if ((*vme_id_names)[i] == name) {
                found_index = i;
                break;
            }
        }

        if (found_index == -1) {
            std::cerr << "Warning: Could not find '" << name << "' in VME beamline_id_name list!" << std::endl;
        }

        outfile << name << "," << card << "," << channel << "," << found_index << "\n";
    }

    outfile.close();
    std::cout << "Mapping file detector_mapping.txt created successfully!" << std::endl;

    vme_file->Close();
    return 0;
}
