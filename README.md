# WCTE Data Analysis Tools

## How to run the main template analysis

```bash
make
./WCTE_DataAnalysis_Template brb_matched_files/WCTE_offline_R1670S0.root boxcuts.json
```

*Change the data path and filename for your setup.*

---

## File Descriptions

### Main Analysis and Configuration

- **WCTE_DataAnalysis_Template.cpp**  
  Main program to run particle ID (PID) and plotting based on QDC and TDC beamline information. Uses `WCTE_BeamMon_PID` for classification and `WCTE_DataQuality` to check run status. Generates a PDF with 1D and 2D plots including total and PID-separated visualizations.

- **WCTE_BeamMon_PID.h / WCTE_BeamMon_PID.cpp**  
  PID classification logic using beamline detector QDC and TDC inputs. Supports box-cut based selection per run, loaded from a JSON configuration. Designed for extensibility to more complex methods.

- **WCTE_DataQuality.h / WCTE_DataQuality.cpp**  
  Run-level quality filter. Currently supports a `"GoodRun"` flag per run from the JSON. Can be extended to enforce timing or channel quality cuts.

- **WCTE_Utility.h / WCTE_Utility.cpp**  
  Utility functions including T0 calibration, mean/sigma extraction via Gaussian fits, and per-event T0 estimation with 3σ filtering. Used in both PMT timing tools.

- **Makefile**  
  Build automation for all programs listed above. Compile with `make`.

- **Generate_DetectorMapping.cpp**  
  Helper tool to parse or create detector channel mapping files.

- **detector_mapping.txt**  
  Example mapping file for beamline PMTs or channels, as used by the mapping generator or waveform readers.

- **boxcuts.json**  
  Configuration file storing PID selection cuts (under `"box"`) and data quality flags (under `"dataquality"`) for each run ID.

---

### Other: Test programs, simple analysis, comparisons

- **WCTE_TPMT_Analysis.cpp**  
  Analyzes hit PMT data (timing and QDC) for a selected card. Computes time-of-flight (ToF) relative to a reference T0 derived from PMTs on card 131 (channels 12–15).

- **WCTE_TOFCardAnalysis.cpp**  
  Structured for broader cross-checking and debugging. Uses the new `WCTE_Utility` class for T0 calibration and per-event T0 computation, with optional debug comparison against old methods.

- **Utility_test.cpp**  
  Standalone tester for T0 calibration and computation. Validates `WCTE_Utility` logic against reference T0s and prints debug output with rejection of outliers.

- **WCTE_BRB_VME_Comparison.cpp**  
  Compares PID histograms (1D and 2D) between BRB and VME readout formats. Useful for debugging or cross-validating both systems.

- **WCTE_BRB_VME_Comparison_EvSelPlots.cpp**  
  Variant of the comparison program focused on event selection–related distributions.

- **BRB_Internal_Comparison.cpp**  
  Produces internal comparisons of beamline data within a single BRB file (e.g. comparing different PMT groups).

- **BRB_hitPMT_plots.cpp**  
  Plots raw PMT waveform data or hit distributions from BRB files.
