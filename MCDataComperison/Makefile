# Compiler and flags
CXX = g++
CXXFLAGS = -O2 -std=c++17

# ROOT flags
ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS   = $(shell root-config --libs)

# WCSim paths and libraries
WCSIM_INC  = /opt/WCSim/build/install/include/WCSim/
WCSIM_LIB  = /opt/WCSim/build/install/lib
WCSIM_LIBS = -lWCSimRoot

# Executable and source
TARGET = WCSim2BRB_Converter
SRCS = WCSim2BRB_Converter.cpp

$(TARGET): $(SRCS)
	$(CXX) $(ROOTCFLAGS) $(CXXFLAGS) -I$(WCSIM_INC) $(SRCS) -o $(TARGET) \
	$(ROOTLIBS) -lstdc++fs -L$(WCSIM_LIB) $(WCSIM_LIBS) -Wl,-rpath,$(WCSIM_LIB)

# Clean rule
clean:
	rm -f $(TARGET)
