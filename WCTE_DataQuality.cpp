#include "WCTE_DataQuality.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

WCTE_DataQuality::WCTE_DataQuality() : current_run_id_(-1) {}

bool WCTE_DataQuality::LoadQualityInfo(const std::string& json_filename) {
    std::ifstream file(json_filename);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file: " << json_filename << std::endl;
        return false;
    }

    json j;
    file >> j;

    for (auto& [runid_str, rundata] : j.items()) {
        int run_id = std::stoi(runid_str);
        if (rundata.contains("dataquality") && rundata["dataquality"].contains("GoodRun")) {
            run_quality_[run_id] = rundata["dataquality"]["GoodRun"].get<bool>();
        } else {
            run_quality_[run_id] = false; // Default to bad run if no info
        }
    }

    return true;
}

void WCTE_DataQuality::SetRunID(int run_id) {
    current_run_id_ = run_id;
}

bool WCTE_DataQuality::IsGoodRun() const {
    auto it = run_quality_.find(current_run_id_);
    if (it != run_quality_.end()) {
        return it->second;
    }
    return false; // Default to bad if not found
}
