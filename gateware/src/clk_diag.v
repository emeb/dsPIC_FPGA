// clk_diag.v - divide clock input by N as visible diagnostic
// 2009-01-15 E. Brombaugh

module clk_diag(clk, reset, out);
	parameter bits = 26;			// bits in divider - must accept period
	parameter period = 12288000;	// 1/2 the actual divide factor
	
	input clk;			// Clock input to check
	input reset;		// POR
	output out;			// diagnostic flash output
	
	// main divider
	reg [bits-1:0] cntr;
	always @(posedge clk)
		if(reset)
			cntr <= period;
		else
		begin
			if(cntr == 0)
				cntr <= period - 1;
			else
				cntr <= cntr - 1 ;
		end
	
	// divide by 2 for visibility
	reg out;
	always @(posedge clk)
		if(reset)
			out <= 0;
		else
		begin
			if(cntr == 0)
				out <= ~out;
		end
endmodule
			