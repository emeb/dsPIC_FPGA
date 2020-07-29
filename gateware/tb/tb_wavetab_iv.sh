#!/bin/sh
# tb_wavetab_iv.sh: simulate the wave table testbench in Icarus Verilog

# compile the design
iverilog -o tb_wavetab tb/tb_wavetab.v \
	src/wavetab.v src/clk_diag.v src/flashspi.v src/i2s_out.v \
	src/spi_slave.v src/wavegen.v src/sat.v \
	/opt/Xilinx/10.1/ISE/verilog/src/unisims/IBUFG.v \
	/opt/Xilinx/10.1/ISE/verilog/src/unisims/DCM_SP.v \
	/opt/Xilinx/10.1/ISE/verilog/src/unisims/FDCE.v \
	/opt/Xilinx/10.1/ISE/verilog/src/glbl.v \
	-y /opt/Xilinx/10.1/ISE/verilog/src/unisims/
#	src/chipscope_icon_v1_03_a.v \
#	src/chipscope_ila_v1_02_a.v \

# simulate the design
vvp tb_wavetab
