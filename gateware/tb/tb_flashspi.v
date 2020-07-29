// tb_flashspi.v: Automatic SPI interface for LTC1867 ADC
// 2008-02-03 E. Brombaugh

`timescale 1ns/1ps

module tb_flashspi;
	parameter asz = 24;			// Address bus size
	parameter dsz = 16;			// Data bus size

	reg clk;					// 24.576MHz system clock
	reg reset;					// POR
	reg wrt_req;				// Request write mux override
	wire wrt_ack;				// Acknowledge write mux request
	reg samp_ena;				// Sample rate enable (1/256)
	wire [1:0] cyc_num;			// Cycle number (0-2, 3 unused)
	wire [asz-1:0] addr;		// Flash Read Address
	wire [dsz-1:0] data;		// Flash Read Data
	wire data_stb;				// Data Valid strobe
	reg wrt_mosi;				// Write SPI MOSI override
	reg wrt_clk;				// Write SPI Clock override
	reg wrt_cs;					// Write SPI CS override
	wire flsh_mosi;				// Flash SPI MOSI
	wire flsh_miso;				// Flash SPI MISO
	wire flsh_clk;				// Flash SPI Clock
	wire flsh_cs;				// Flash SPI CS
	
	flashspi
		uut(.clk(clk), .reset(reset),
			.wrt_req(wrt_req), .wrt_ack(wrt_ack), .samp_ena(samp_ena), 
			.cyc_num(cyc_num), .addr(addr), .data(data), .data_stb(data_stb),
			.wrt_mosi(wrt_mosi),
			.wrt_clk(wrt_clk), .wrt_cs(wrt_cs),
			.flsh_mosi(flsh_mosi), .flsh_miso(flsh_miso),
			.flsh_clk(flsh_clk), .flsh_cs(flsh_cs));
	
	// Shift register to delay mosi->miso by 32
	reg [31:0] dly;
	always @(posedge flsh_clk)
		dly <= {dly[30:0],flsh_mosi};
	
	// Falling edge triggered SR output to mimic real SPI device
	reg dly_fe;
	always @(negedge flsh_clk)
		dly_fe <= dly[31];
		
	assign flsh_miso = dly_fe;
	
	// changing address
	assign addr = {6'h00,cyc_num,16'hBEEF};
	
	// ~24.576MHz clock
	initial
		clk = 0;
	always
		#20 clk = ~clk;
	
	// POR
	initial
	begin
		$dumpfile("tb_flashspi.vcd");
		$dumpvars(0, tb_flashspi);
		reset = 1;
		#108
		reset = 0;
	end
	
	// runlength
	integer count;
	initial
		count = 0;
	
	always @(posedge clk)
	begin
		count <= count + 1;
		
		if(count == 2000)
			$finish;
		
		// Sample rate clock enable
		if(count%256 == 255)
			samp_ena <= 1'b1;
		else
			samp_ena <= 1'b0;
	end
	
	// stimulus
	initial
	begin
		reset = 1'b1;
		wrt_req = 1'b0;
		samp_ena = 1'b0;
		wrt_mosi = 1'b0;
		wrt_clk = 1'b0;
		wrt_cs = 1'b1;
		
		// release reset
		#100
		reset = 1'b0;
		
		// Assert Write Req
		#34900
		wrt_req = 1'b1;
		
		// De-assert Write Req
		#10000
		wrt_req = 1'b0;
		
	end
endmodule
