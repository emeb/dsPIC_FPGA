// sine.v: lookup sine and cosine values from phase
// 2006-08-24 E. Brombaugh

module sine(clk, phs, sin);
	parameter psz = 12;				// Bits in phase word
	parameter osz = 18;				// Bits in output words
	
	input clk;						// Main system clock (122 MHz)
	input signed [psz-1:0] phs;		// Output of phase acc
	output signed [osz-1:0] sin;	// output, 4x TDM
		
	// Split phase up into contols bits
	wire sai, sdi;					// Address/Data inversion bits
	assign sai = phs[psz-2];		// invert sine address in odd quads
	assign sdi = phs[psz-1];		// invert sine data in back half
	
	// Invert phs lsbs to create sin & cos addresses
	wire [psz-3:0] sadd;			// sin & cos addresses
	assign sadd = {psz-2{sai}} ^ phs[psz-3:0];

	// delay data invert bits
	reg sdid0, sdid1;				// delayed data invert bits
	always @(posedge clk)
	begin
		sdid0 <= sdi;
		sdid1 <= sdid0;
	end
	
	// Lookup raw sin data (2 cycle delay)
	wire signed [osz-1:0] rsin;		// raw value (all positive)
	sc_lut #(.asz(psz-2), .dsz(osz))
		u0(.clk(clk), .a(sadd), .d(rsin));
	
	// async invert data outputs
	reg signed [osz-1:0] psin;		// unregistered outputs
	always @(rsin or sdid1)
		psin = ({osz{sdid1}} ^ rsin) + {{osz{1'b0}},sdid1};
	
	// Sync output registers
	reg signed [osz-1:0] sin;		// output
	always @(posedge clk)
		sin <= psin;
endmodule

	
