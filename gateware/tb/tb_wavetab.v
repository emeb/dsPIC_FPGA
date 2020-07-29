// tb_i2s_tst_top.v - trial testbench to checkout Xilinx ISE Simulator
// 11-19-2008 E. Brombaugh

`timescale 1ns/1ps

module tb_wavetab;

	reg FPGA_CLK;			// Main 24.576MHz clock osc

	// MCU interface
	reg FPGA_CSL;			// SPI CS
	reg FPGA_MOSI;		// SPI data in
	wire FPGA_MISO;		// SPI data out
	reg FPGA_SCLK;		// SPI clock
	wire FPGA_IRQ;		// IRQ to MCU
	reg FPGA_SPI_REQ;		// Request SPI MUX
	wire FPGA_SPI_ACK;	// Grant SPI MUX 

	// Flash Interface
	wire FLASH_SCK;		// Flash SPI clock
	wire FLASH_SI;		// Flash SPI data output to memory
	wire FLASH_SO;		// Flash SPI data input from memory
	wire FLASH_WP;		// Flash SPI write protect
	wire FLASH_CS;		// Flash SPI CS
	wire FLASH_RST;		// Flash SPI reset

	// CODEC Interface
	wire DAC_MCLK;		// Master clock
	wire DAC_SCLK;		// Bit clock
	wire DAC_LRCK;		// Word clock
	wire DAC_SDIN;		// Serial Data to DAC

	// Digilent connectors
	wire [3:0] AUX1;		// all wires for now
	wire [3:0] AUX2;		// all wires for now
	wire [3:0] AUX3;		// all wires for now

	// LED
	wire FPGA_ACT;			// activity / debug

	// Xilinx needs this
	glbl glbl();

	// Unit under test	
	wavetab
	uut(
		// Clocks
		.FPGA_CLK(FPGA_CLK),			// Main 24.576MHz clock osc
				
		// MCU interface
		.FPGA_CSL(FPGA_CSL),			// SPI CS
		.FPGA_MOSI(FPGA_MOSI),			// SPI data in
		.FPGA_MISO(FPGA_MISO),			// SPI data out
		.FPGA_SCLK(FPGA_SCLK),			// SPI clock
		.FPGA_IRQ(FPGA_IRQ),			// IRQ to MCU
		.FPGA_SPI_REQ(FPGA_SPI_REQ),	// Request SPI MUX
		.FPGA_SPI_ACK(FPGA_SPI_ACK),	// Grant SPI MUX 
		
		// Flash Interface
		.FLASH_SCK(FLASH_SCK),			// Flash SPI clock
		.FLASH_SI(FLASH_SI),			// Flash SPI data in
		.FLASH_SO(FLASH_SO),			// Flash SPI data out
		.FLASH_WP(FLASH_WP),			// Flash SPI write protect
		.FLASH_CS(FLASH_CS),			// Flash SPI CS
		.FLASH_RST(FLASH_RST),			// Flash SPI reset
		
		// CODEC Interface
		.DAC_MCLK(DAC_MCLK),			// Master clock
		.DAC_SCLK(DAC_SCLK),			// Bit clock
		.DAC_LRCK(DAC_LRCK),			// Word clock
		.DAC_SDIN(DAC_SDIN),			// Serial Data to DAC
		
		// Digilent connectors
		.AUX1(AUX1),					// all wires for now
		.AUX2(AUX2),					// all wires for now
		.AUX3(AUX3),					// all wires for now
		
		// LED
		.FPGA_ACT(FPGA_ACT)				// activity / debug
	);
	
	// test dummy of flash memory
	spi_flash
		umem(
		.CLK(FLASH_SCK),		// SPI Clock
		.CS(FLASH_CS),			// SPI CS
		.SI(FLASH_SI),			// SPI Data In
		.SO(FLASH_SO));			// SPI Data Out
	
	// Clock source
	always
		#20 FPGA_CLK = ~FPGA_CLK;
	
	// setup
	initial
	begin
		$dumpfile("tb_wavetab.vcd");
		$dumpvars(0, tb_wavetab);
		
		FPGA_CLK = 1'b0;
		FPGA_CSL = 1'b1;			// SPI CS
		FPGA_MOSI = 1'b0;			// SPI data in
		FPGA_SCLK = 1'b0;			// SPI clock
		FPGA_SPI_REQ = 1'b0;		// Request SPI MUX
		//FLASH_SO = 1'b0;			// Flash SPI data out
		
		// Temporary
		//force uut.l_data = 24'hBEEF01;
		//force uut.r_data = 24'haaaaaa;
		force uut.freq = 32'h00400000;
		force uut.wave1 = 10'h100;
		force uut.wave2 = 10'h200;
		
		//#50000
		//FPGA_SPI_REQ = 1'b1;		// Request SPI MUX
		
		#100000
		$finish;
	end
endmodule

// Model of AT45DB321D SPI Flash Memory in read mode
module spi_flash(
	input CLK,			// SPI Clock
	input CS,			// SPI CS
	input SI,			// SPI Data In
	output reg SO);		// SPI Data Out
	
	parameter dsz = 16;
	parameter asz = 24;
	
	reg [5:0] bcnt;
	always @(posedge CLK or posedge CS)
		if(CS)
			bcnt <= 0;
		else
			bcnt <= bcnt + 1;
			
	// Shift register to delay mosi->miso by 32
	reg [31:0] dly;
	wire [31:0] data;
	always @(posedge CLK)
		if(bcnt == 6'h1f)	// cmd/add loaded, load 2k saw
			dly <= data;
		else
			dly <= {dly[30:0],SI};
	
	// address is concatenated data bits
	wire [asz-1:0] addr = {dly[22:0],SI};
	
	// index is re-arranged address - paging, 2byte/word etc
	wire [10:0] idx = {addr[13:10],addr[8:1]};
	
	// build 1st & 2nd data words
	wire [dsz-1:0] word1, word2;
	assign word1 = idx<<5;
	assign word2 = (idx+1)<<5;
	
	// concatenate
	assign data = {word1,word2};
	
	// Falling edge triggered SR output to mimic real SPI device
	always @(negedge CLK)
		SO <= dly[31];
		
endmodule
