#!/bin/sh
# tb_flashspi_iv.sh: simulate the Flash SPI testbench in Icarus Verilog

# compile the design
iverilog -o tb_flashspi tb/tb_flashspi.v src/flashspi.v \
	-y /opt/Xilinx/10.1/ISE/verilog/src/unisims/

# simulate the design
vvp tb_flashspi



