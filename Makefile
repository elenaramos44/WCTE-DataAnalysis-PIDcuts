CXX = g++
CXXFLAGS = `root-config --cflags` -O2 -std=c++17
LDLIBS = `root-config --libs`

TARGETS = \
    WCTE_BRB_VME_Comparison \
    WCTE_BRB_VME_Comparison_EvSelPlots \
    GenerateMapping \
    BRB_hitPMT_plots \
    BRB_Internal_Comparison \
    WCTE_DataAnalysis_Template \
    WCTE_TPMT_Analysis \
    WCTE_TOFCardAnalysis \
    Utility_test

all: $(TARGETS)

WCTE_BRB_VME_Comparison: WCTE_BRB_VME_Comparison.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

WCTE_BRB_VME_Comparison_EvSelPlots: WCTE_BRB_VME_Comparison_EvSelPlots.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

GenerateMapping: Generate_DetectorMapping.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

BRB_hitPMT_plots: BRB_hitPMT_plots.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

BRB_Internal_Comparison: BRB_Internal_Comparison.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

WCTE_DataAnalysis_Template: WCTE_DataAnalysis_Template.cpp WCTE_BeamMon_PID.cpp WCTE_DataQuality.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

WCTE_TPMT_Analysis: WCTE_TPMT_Analysis.cpp WCTE_Utility.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

WCTE_TOFCardAnalysis: WCTE_TOFCardAnalysis.cpp WCTE_BeamMon_PID.cpp 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

Utility_test: Utility_test.cpp WCTE_BeamMon_PID.cpp WCTE_Utility.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGETS) *.o *.pdf

.PHONY: all clean
