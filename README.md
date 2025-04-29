
# WCTE Data Analysis Tools

## How to run the main template analysis

```bash
make
./WCTE_DataAnalysis_Template brb_matched_files/WCTE_offline_R1670S0.root boxcuts.json
```

*Change the data path and filename for your setup.*

---

## File Descriptions

### Main Analysis & Configuration

- **WCTE_DataAnalysis_Template.cpp**  
  Main program to run particle ID (PID) and plotting based on QDC and TDC beamline information. Uses `WCTE_BeamMon_PID` for classification and `WCTE_DataQuality` to check run status. Generates a PDF with 1D and 2D plots including total and PID-separated visualizations.

- **WCTE_BeamMon_PID.h / WCTE_BeamMon_PID.cpp**  
  PID classification logic using beamline detector QDC and TDC inputs. Supports box-cut based selection per run, loaded from a JSON configuration. Designed for extensibility to more complex methods.

- **WCTE_DataQuality.h / WCTE_DataQuality.cpp**  
  Run-level quality filter. Currently supports a "GoodRun" flag per run from the JSON. Can be extended to enforce timing or channel quality cuts.

- **boxcuts.json**  
  Configuration file storing PID selection cuts (under `"box"`) and data quality flags (under `"quality"`) for each run ID. Example structure:
  ```json
  {
    "1670": {
      "box": {
        "electron": { "tof_min": 10.0, "tof_max": 15.0, "act_min": 0.0, "act_max": 1000.0 },
        "muon":     { "tof_min": 15.0, "tof_max": 18.0, "act_min": 0.0, "act_max": 1000.0 },
        "pion":     { "tof_min": 18.0, "tof_max": 22.0, "act_min": 1000.0, "act_max": 5000.0 }
      },
      "quality": {
        "GoodRun": true
      }
    }
  }
  ```

---

### Other Utilities

- **WCTE_BRB_VME_Comparison.cpp**  
  Compares PID histograms (1D and 2D) between BRB and VME readout formats. Useful for debugging or cross-validating both systems.

- **WCTE_BRB_VME_Comparison_EvSelPlots.cpp**  
  Variant of the comparison program focused on event selection–related distributions.

- **Generate_DetectorMapping.cpp**  
  Helper tool to parse or create detector channel mapping files.

- **BRB_hitPMT_plots.cpp**  
  Plots raw PMT waveform data or hit distributions from BRB files.

- **BRB_Internal_Comparison.cpp**  
  Produces internal comparisons of beamline data within a single BRB file (e.g. comparing different PMT groups).

- **Makefile**  
  Build automation for all programs listed above. Compile with `make`.

- **detector_mapping.txt**  
  Example mapping file for beamline PMTs or channels, as used by the mapping generator or waveform readers.
