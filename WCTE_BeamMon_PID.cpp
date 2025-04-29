#include "WCTE_BeamMon_PID.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

WCTE_BeamMon_PID::WCTE_BeamMon_PID() {}

void WCTE_BeamMon_PID::SetRunID(int run_id) {
    current_run_id_ = run_id;
}

void WCTE_BeamMon_PID::SetBeamlineData(const std::vector<float>* qdc_charge,
                                       const std::vector<int>*   qdc_ids,
                                       const std::vector<float>* tdc_time,
                                       const std::vector<int>*   tdc_ids) {
    qdc_charge_ = qdc_charge;
    qdc_ids_    = qdc_ids;
    tdc_time_   = tdc_time;
    tdc_ids_    = tdc_ids;
}

bool WCTE_BeamMon_PID::LoadBoxCuts(const std::string& json_filename) {
    std::ifstream file(json_filename);
    if (!file.is_open()) {
        std::cerr << "Error opening boxcuts file: " << json_filename << std::endl;
        return false;
    }

    json j;
    file >> j;

    for (auto& [runid_str, rundata] : j.items()) {
        int run_id = std::stoi(runid_str);
        BoxDefinitions def;
        def.electron.t0t1_min = rundata["box"]["electron"]["tof_min"];
        def.electron.t0t1_max = rundata["box"]["electron"]["tof_max"];
        def.electron.act_min  = rundata["box"]["electron"]["act_min"];
        def.electron.act_max  = rundata["box"]["electron"]["act_max"];

        def.muon.t0t1_min = rundata["box"]["muon"]["tof_min"];
        def.muon.t0t1_max = rundata["box"]["muon"]["tof_max"];
        def.muon.act_min  = rundata["box"]["muon"]["act_min"];
        def.muon.act_max  = rundata["box"]["muon"]["act_max"];

        def.pion.t0t1_min = rundata["box"]["pion"]["tof_min"];
        def.pion.t0t1_max = rundata["box"]["pion"]["tof_max"];
        def.pion.act_min  = rundata["box"]["pion"]["act_min"];
        def.pion.act_max  = rundata["box"]["pion"]["act_max"];

        run_boxcuts_[run_id] = def;
    }
    return true;
}

bool WCTE_BeamMon_PID::EventPassesCuts() const {
    if (!qdc_charge_ || !qdc_ids_ || !tdc_time_ || !tdc_ids_) return false;

    double t0 = 0, t1 = 0;
    int t0hits = 0, t1hits = 0;
    bool t4_hit = false;
    bool hole0 = false, hole1 = false;
    double act_sum = 0;

    // Process QDC information
    for (size_t j = 0; j < qdc_ids_->size(); ++j) {
        int ch = (*qdc_ids_)[j];
        float qdc = (*qdc_charge_)[j];
        if (ch == 42 || ch == 43) if (qdc > 300) t4_hit = true;
        if (ch == 9 && qdc > 150) hole0 = true;
        if (ch == 10 && qdc > 100) hole1 = true;
        if (ch >= 18 && ch <= 23) act_sum += qdc;
    }

    if (!t4_hit || hole0 || hole1) return false;

    // Process TDC information
    for (size_t j = 0; j < tdc_ids_->size(); ++j) {
        int ch = (*tdc_ids_)[j];
        float tdc = (*tdc_time_)[j] - 250.0;  // Subtract 250 ns baseline
        if (ch >= 0 && ch <= 3 && tdc < -100) { t0 += tdc; ++t0hits; }
        if (ch >= 4 && ch <= 7 && tdc < -100) { t1 += tdc; ++t1hits; }
    }

    return (t0hits == 4 && t1hits == 4);
}

double WCTE_BeamMon_PID::computeT0Avg() const {
    double t0 = 0;
    int t0hits = 0;

    if (!tdc_time_ || !tdc_ids_) return -999;

    for (size_t j = 0; j < tdc_ids_->size(); ++j) {
        int ch = (*tdc_ids_)[j];
        float tdc = (*tdc_time_)[j] - 250.0;
        if (ch >= 0 && ch <= 3 && tdc < -100) {
            t0 += tdc;
            ++t0hits;
        }
    }

    return (t0hits == 4) ? t0 / 4.0 : -999;
}

double WCTE_BeamMon_PID::computeT1Avg() const {
    double t1 = 0;
    int t1hits = 0;

    if (!tdc_time_ || !tdc_ids_) return -999;

    for (size_t j = 0; j < tdc_ids_->size(); ++j) {
        int ch = (*tdc_ids_)[j];
        float tdc = (*tdc_time_)[j] - 250.0;
        if (ch >= 4 && ch <= 7 && tdc < -100) {
            t1 += tdc;
            ++t1hits;
        }
    }

    return (t1hits == 4) ? t1 / 4.0 : -999;
}

double WCTE_BeamMon_PID::computeActGroup2Sum() const {
    if (!qdc_charge_ || !qdc_ids_) return -1;

    double sum = 0;
    for (size_t j = 0; j < qdc_ids_->size(); ++j) {
        int ch = (*qdc_ids_)[j];
        float val = (*qdc_charge_)[j];
        if (ch >= 18 && ch <= 23) sum += val;
    }
    return sum;
}

double WCTE_BeamMon_PID::GetTofT0T1() const {
    double t0 = computeT0Avg();
    double t1 = computeT1Avg();
    return (t0 != -999 && t1 != -999) ? (t1 - t0) : -999;
}

double WCTE_BeamMon_PID::GetActGroup2Sum() const {
    return computeActGroup2Sum();
}

int WCTE_BeamMon_PID::GetParticleIDBox() const {
    if (!EventPassesCuts()) return 0;

    double tof = GetTofT0T1();
    double act = GetActGroup2Sum();
    if (tof == -999 || act < 0) return 0;
    if (!run_boxcuts_.count(current_run_id_)) return 0;

    const BoxDefinitions& cuts = run_boxcuts_.at(current_run_id_);

    if (tof >= cuts.electron.t0t1_min && tof <= cuts.electron.t0t1_max &&
        act >= cuts.electron.act_min && act <= cuts.electron.act_max) {
        return 11; // electron
    }
    if (tof >= cuts.muon.t0t1_min && tof <= cuts.muon.t0t1_max &&
        act >= cuts.muon.act_min && act <= cuts.muon.act_max) {
        return 13; // muon
    }
    if (tof >= cuts.pion.t0t1_min && tof <= cuts.pion.t0t1_max &&
        act >= cuts.pion.act_min && act <= cuts.pion.act_max) {
        return 211; // pion
    }

    return 0;
}
