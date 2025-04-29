# WCTE Data Analysis

## Project Structure

| Filename | Description |
|:---|:---|
| **boxcuts.json** | JSON file containing particle identification (PID) cuts per run and method (currently "box" method). Used by `WCTE_BeamMon_PID`. |
| **BRB_hitPMT_plots.cpp** | Creates hit distributions per PMT channel from BRB data (QDCs and TDCs). Useful for checking hit maps. |
| **BRB_Internal_Comparison.cpp** | Compares different internal variables within the same BRB file to study consistency of the readout. |
| **detector_mapping.txt** | Text file defining mapping between PMT channels and detector elements. |
| **Generate_DetectorMapping.cpp** | Generates detector mapping files for reference and matching between PMTs and physical detectors. |
| **Makefile** | Build file to compile all programs with a single command (`make`). Also handles cleaning. |
| **WCTE_BRB_VME_Comparison.cpp** | Compares BRB and VME QDC/TDC readouts by filling histograms channel-by-channel and per event. |
| **WCTE_BRB_VME_Comparison_EvSelPlots.cpp** | Compares BRB and VME T0/T1 event selection plots, focusing on PID and ACT-QDC correlations. |
| **WCTE_DataAnalysis_Template.cpp** | Template program to apply PID using BRB data and `WCTE_BeamMon_PID`, and generate high-level selection plots (ACT3-5 vs TOF and projections). |
| **WCTE_BeamMon_PID.h / WCTE_BeamMon_PID.cpp** | C++ class to calculate PID variables (TOF, ACT sums) and classify particle types using external box cuts (`boxcuts.json`). |

---

## Detailed Descriptions

### `WCTE_BeamMon_PID.h` and `WCTE_BeamMon_PID.cpp`
- This class reads BRB beamline data (`qdc`, `tdc`, `qdc_ids`, `tdc_ids`) and computes:
  - `TofT0T1` = Time-of-flight between T0 and T1.
  - `ActGroup2Sum` = Summed QDC for ACT3-5 detectors.
- Supports **run-dependent box cuts** defined in a JSON file (`boxcuts.json`).
- Identifies particles (electron, muon, pion) based on the loaded box cuts.
- Can easily be extended in the future (e.g., add "polynomial" or "ML-based" PID methods).
- Main methods:
  - `LoadBoxCuts(filename)`: Load box cuts from JSON.
  - `SetRunID(run_id)`: Set current run.
  - `SetBeamlineData(qdc, qdc_ids, tdc, tdc_ids)`: Provide event data.
  - `GetTofT0T1()`: Return computed TOF.
  - `GetActGroup2Sum()`: Return computed ACT3-5 sum.
  - `GetParticleID()`: Return PDG code (11=electron, 13=muon, 211=pion, 0=unknown).

---

### `boxcuts.json`
- JSON format defining PID selection cuts for each run and method.
- Example format:
```json
{
    "1670": {
        "box": {
            "electron": { "tof_min": 10.0, "tof_max": 15.0, "act_min": 0.0, "act_max": 1000.0 },
            "muon":    { "tof_min": 15.0, "tof_max": 18.0, "act_min": 0.0, "act_max": 1000.0 },
            "pion":    { "tof_min": 18.0, "tof_max": 22.0, "act_min": 1000.0, "act_max": 5000.0 }
        }
    }
}
