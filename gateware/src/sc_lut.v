// sc_lut.v: Sine/Cosine Lookup Table
// 2006-07-02 E. Brombaugh

module	sc_lut(clk, a, d);
	parameter asz = 10;				// bits in input address words
	parameter dsz = 18;				// bits in output data word
	
	input clk;						// System clock (122 MHz)
	input [asz-1:0] a;				// input address
	output signed [dsz-1:0] d;		// output data
		
	// delay addresses
	reg [9:0] ADDR;
	always @(posedge clk)
		ADDR <= a;
	
	// Xilinx Block RAM model of LUT
	wire [15:0] DO;
	wire [1:0] DOP;
	RAMB16_S18 #(	
// The following file specifies the initial contents of the RAM
`include "src/sc_lut_init.v"
		)
	RAMB16_inst (
		.DO(DO),
		.DOP(DOP),
		.ADDR(ADDR),
		.CLK(clk),
		.DI(16'hffff),
		.DIP(2'b11),
		.EN(1'b1),
		.SSR(1'b0),
		.WE(1'b0));
	
	// Combine the outputs correctly
	wire signed [17:0] ram_out;
	assign ram_out = {DOP,DO};
	
	// Pick off msbs
	assign d = ram_out[17:18-dsz];
endmodule
