# Directory Structure
TGT_TTB_DIR :=../../../tgt-ttb
SCRIPTS     :=../../../scripts

# Configurations
CLK_BASENAME := clk

all: script

# VCD/Dot Analysis Script
script: tgt-ttb $(TARGET).dot $(TARGET).vcd
	@time pypy $(SCRIPTS)/analyze.py $(filter-out $<,$^)

# IVL Simulation (Step 2: VCD Generation)
$(TARGET).vcd: $(TARGET).vvp
	@vvp $<

# IVL Simulation (Step 1: Executable Generation)
$(TARGET).vvp: $(SOURCES) $(TESTBENCH)
	@iverilog -t vvp -o $@ $(INCLUDEDIRS) $^

# IVL Target TTB Module Analysis
$(TARGET).dot: $(SOURCES) $(TESTBENCH)
	@echo "Generating DOT graph..." && \
	time iverilog -o $@ -pclk=$(CLK_BASENAME) -t ttb $(INCLUDEDIRS) $^

.PHONY: clean tgt-ttb

# Building the IVL Target Module
tgt-ttb: 
	$(MAKE) -C $(TGT_TTB_DIR) -j8

cleanall: clean
	$(MAKE) cleanall -C $(TGT_TTB_DIR)

clean:
	$(RM) $(TARGET).vvp $(TARGET).dot $(TARGET).vcd
