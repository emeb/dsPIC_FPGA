// i2s_out.v: 1/256 rate 24 bit stereo I2S output
// 2010-02-09 E. Brombaugh

module i2s_out(clk, reset,
				l_data, r_data,
				sdout, sclk, lrclk, mclk,
				load);
	
	input clk;									// System clock
	input reset;								// System POR
	input signed [23:0] l_data, r_data;			// inputs
	output sdout;								// I2S serial data
	output sclk;								// I2S serial clock
	output lrclk;								// I2S Left/Right clock
	output mclk;								// I2S Master clock
	output load;								// Sample rate enable output
	
	// rate counter drives everything
	reg load;
	reg [7:0] cnt;
	always @(posedge clk)
		if(reset)
			cnt <= 8'h00;
		else
			cnt <= cnt + 1;
	
	// detect end condition for sample rate
	always @(posedge clk)
		load <= &cnt;
		
	// generate serial clock pulse
	reg p_sclk;			// 1 cycle wide copy of serial clock
	always @(posedge clk)
		p_sclk <= &cnt[1:0];
	
	// Shift register advances on serial clock
	reg [63:0] sreg;
	always @(posedge clk)
		if(load)
			sreg <= {l_data,8'h00,r_data,8'h00};
		else if(p_sclk)
			sreg <= {sreg[62:0],1'b0};
	
	// I2S needs 1 serial clock cycle delay on data relative to LRCLK
	reg sdout;
	always @(posedge clk)
		if(p_sclk)
			sdout <= sreg[63];
	
	// Generate LRCLK
	reg lrclk;
	always @(posedge clk)
		lrclk <= ~cnt[7];
	
	// Gate MCLK
	assign mclk = clk;
	
	// Generate SCLK
	reg sclk;
	always @(posedge clk)
		sclk <= cnt[1];
endmodule
