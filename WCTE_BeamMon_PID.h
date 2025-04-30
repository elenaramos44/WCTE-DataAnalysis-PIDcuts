#ifndef WCTE_BEAMMON_PID_H
#define WCTE_BEAMMON_PID_H

#include <vector>
#include <string>
#include <map>

class WCTE_BeamMon_PID {
public:
    WCTE_BeamMon_PID();

    void SetRunID(int run_id);
    void SetBeamlineData(const std::vector<float>* qdc_charge,
                         const std::vector<int>*   qdc_ids,
                         const std::vector<float>* tdc_time,
                         const std::vector<int>*   tdc_ids);

    bool LoadBoxCuts(const std::string& json_filename);
    void SetPIDMethod(const std::string& method);
    
    double GetTofT0T1() const;
    double GetActGroup2Sum() const;
    int GetParticleID() const;      // Dispatching function
    int GetParticleIDBox() const;   // Box cut logic
    bool EventPassesCuts() const;

private:
    struct ParticleCuts {
        double t0t1_min, t0t1_max;
        double act_min, act_max;
    };

    struct BoxDefinitions {
        ParticleCuts electron;
        ParticleCuts muon;
        ParticleCuts pion;
    };

    int current_run_id_ = -1;
    std::string pid_method_ = "box";  // Default

    std::map<int, BoxDefinitions> run_boxcuts_;

    const std::vector<float>* qdc_charge_ = nullptr;
    const std::vector<int>*   qdc_ids_    = nullptr;
    const std::vector<float>* tdc_time_   = nullptr;
    const std::vector<int>*   tdc_ids_    = nullptr;

    double computeT0Avg() const;
    double computeT1Avg() const;
    double computeActGroup2Sum() const;
};

#endif
