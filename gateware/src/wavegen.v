// wavegen.v - stereo audio data generator for wavetables
// E. Brombaugh - 02-08-2010

module wavegen
	#(
		parameter fsz = 32,		// frequency word size
		osz = 24,				// output word size
		psz = 12,				// bits on phase
		wsz = 16				// waveform size
	)
	(
		input clk,								// system clock
		input reset,							// POR
		input ena,								// Sample rate enable
		input [fsz-1:0] freq,					// freq_ctl
		input [9:0] wave0, wave1, wave2,		// wave selects
		input [15:0] blend01,					// Blend waves 0 & 1
		input [1:0] cyc_num,					// access cycle number
		output reg [23:0] addr,					// flash address
		input signed [wsz-1:0] data, 			// flash read data
		input data_stb,							// read data valid strobe
		output reg signed [osz-1:0] l_data,		// left audio data output
		output reg signed [osz-1:0] r_data		// right audio data output
	);
	
	// additional parameters
	parameter isz = 18;						// interp size
	parameter msz = isz+wsz;				// MAC size
	parameter rnd = 1 << (isz-(osz-wsz)-2);	// round bit
	
	// NCO
	reg [fsz-1:0] phase;
	always @(posedge clk)
		if(reset)
			phase <= {fsz{1'b0}};
		else if(ena)
			phase <= phase + freq;
	
	// Mux Wave address
	always @(*)
		case(cyc_num)
			2'b00: addr = {1'b0,wave0,phase[31:29],1'b0,phase[28:21],1'b0};
			2'b01: addr = {1'b0,wave1,phase[31:29],1'b0,phase[28:21],1'b0};
			2'b10: addr = {1'b0,wave2,phase[31:29],1'b0,phase[28:21],1'b0};
			2'b11: addr = {1'b0,wave0,phase[31:29],1'b0,phase[28:21],1'b0};
		endcase
	
	// read data cycle counter
	reg [2:0] ccnt;
	always @(posedge clk)
		if(reset | ena)
			ccnt <= 3'b0;
		else if(data_stb)
			ccnt <= ccnt + 1;
	
	// delay the strobe 
	reg [5:0] dly_stb;
	always @(posedge clk)
		dly_stb <= {dly_stb[4:0],data_stb};
	
	// grab incoming read data and fractional phase
	reg signed [wsz-1:0] xdat;
	reg signed [isz-1:0] frac;
	always @(posedge clk)
		if(data_stb)
		begin
			xdat <= data;
			frac <= ccnt[0] ? {1'b0,phase[20:4]} : {1'b0,~phase[20:4]};
		end
	
	// multiply read data by frac
	reg signed [msz-1:0] m;
	always @(posedge clk)
		if(dly_stb[0])
			m <= xdat * frac;
	
	// accumulate result
	reg signed [msz-1:0] acc;
	always @(posedge clk)
		if(dly_stb[1])
			acc <= m + (ccnt[0] ? rnd : acc);
	
	// truncate
	wire signed [osz-1:0] tacc = acc>>>(isz-(osz-wsz)-1);
	
	// grap interpolated data
	reg signed [osz-1:0] rd0, rd1, rd2;
	always @(posedge clk)
		if(reset)
		begin
			rd0 <= 0;
			rd1 <= 0;
			rd2 <= 0;
		end
		else if(dly_stb[2] & ~ccnt[0])
			case(ccnt[2:1])
				2'h1: rd0 <= tacc;
				2'h2: rd1 <= tacc;
				2'h3: rd2 <= tacc;
			endcase
		
	// blend waves 0 & 1
	reg signed [17:0] bx, bf;
	always @(posedge clk)
		if(dly_stb[2] & ~ccnt[0])
		begin
			bx <= tacc[osz-1:osz-18];
			bf <= ccnt[1] ? {1'b0,~blend01,1'b1} : {1'b0,blend01,1'b0};
		end
	
	// multiply
	reg signed [35:0] bmul;
	always @(posedge clk)
		bmul <= bx * bf;
	
	// combine
	reg signed [35:0] bacc;
	always @(posedge clk)
		if(dly_stb[4]  & ~ccnt[0])
			bacc <= bmul + (cyc_num[0] ?(1<<10) : bacc);
	
	// drop lsbs & saturate
	wire signed [osz-1:0] tbacc;
		sat #(.isz(25), .osz(osz))
			ubsat(.in(bacc[35:11]), .out(tbacc));
	
	// Grab blended data
	reg signed [osz-1:0] bw01;
	always @(posedge clk)
		if(reset)
			bw01 <= 0;
		else if(dly_stb[5] & (ccnt == 3'b100))
			bw01 <= tbacc;
		
	// Outputs shifted up 6x for overhead
	always @(posedge clk)
		if(ena)
		begin
			l_data <= bw01;				// Blended Wave 0 & 1
			r_data <= rd2;				// Wave 2
		end
endmodule
