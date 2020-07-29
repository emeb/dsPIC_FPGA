// wavetab.v - top-level FPGA dsPIC_fpga flash wavetable
// 02-08-10 E. Brombaugh

module wavetab
	(
		// Clocks
		input FPGA_CLK,			// Main 24.576MHz clock osc
				
		// MCU interface
		input FPGA_CSL,			// SPI CS
		input FPGA_MOSI,		// SPI data in
		output FPGA_MISO,		// SPI data out
		input FPGA_SCLK,		// SPI clock
		output FPGA_IRQ,		// IRQ to MCU
		input FPGA_SPI_REQ_L,	// Request SPI MUX
		output FPGA_SPI_ACK_L,	// Grant SPI MUX 
		
		// Flash Interface
		output FLASH_SCK,		// Flash SPI clock
		output FLASH_SI,		// Flash SPI data in
		input FLASH_SO,			// Flash SPI data out
		output FLASH_WP,		// Flash SPI write protect
		output FLASH_CS,		// Flash SPI CS
		output FLASH_RST,		// Flash SPI reset
		
		// CODEC Interface
		output DAC_MCLK,		// Master clock
		output DAC_SCLK,		// Bit clock
		output DAC_LRCK,		// Word clock
		output DAC_SDIN,		// Serial Data to DAC
		
		// Digilent connectors
		output [3:0] AUX1,		// all outputs for now
		output [3:0] AUX2,		// all outputs for now
		output [3:0] AUX3,		// all outputs for now
		
		// LED
		output FPGA_ACT			// activity / debug
	);

	// This should be unique so firmware knows who it's talking to
	parameter DESIGN_ID = 32'hBEEF1005;

	// Clock input buffer
	wire clkin, reset;
	IBUFG
		uclkbuf(.I(FPGA_CLK), .O(clkin));
	
	// Clock retimer
	wire clk, clk2;
	DCM_SP #(
			.CLKDV_DIVIDE(2.0),				// Divide by: 1.5,2.0,2.5,3.0,3.5,4.0,4.5,5.0,5.5,6.0,6.5
											// 7.0,7.5,8.0,9.0,10.0,11.0,12.0,13.0,14.0,15.0 or 16.0
			.CLKFX_DIVIDE(1),				// Can be any integer from 1 to 32
			//.CLKFX_MULTIPLY(4),				// Can be any integer from 2 to 32
			//.CLKIN_DIVIDE_BY_2("FALSE"),	// TRUE/FALSE to enable CLKIN divide by two feature
			//.CLKIN_PERIOD(40.0),			// Specify period of input clock
			//.CLKOUT_PHASE_SHIFT("NONE"),	// Specify phase shift of NONE, FIXED or VARIABLE
			//.CLK_FEEDBACK("1X"),			// Specify clock feedback of NONE, 1X or 2X
			//.DESKEW_ADJUST("SYSTEM_SYNCHRONOUS"), // SOURCE_SYNCHRONOUS, SYSTEM_SYNCHRONOUS or
											// an integer from 0 to 15
			//.DLL_FREQUENCY_MODE("LOW"),		// HIGH or LOW frequency mode for DLL
			//.DUTY_CYCLE_CORRECTION("TRUE"),	// Duty cycle correction, TRUE or FALSE
			.PHASE_SHIFT(0),				// Amount of fixed phase shift from -255 to 255
			.STARTUP_WAIT("TRUE")			// Delay configuration DONE until DCM LOCK, TRUE/FALSE
		)
		uclk_dcm (
			.CLK0(clk),						// 0 degree DCM CLK output
			.CLK180(),						// 180 degree DCM CLK output
			.CLK270(),						// 270 degree DCM CLK output
			.CLK2X(clk2),					// 2X DCM CLK output
			.CLK2X180(),					// 2X, 180 degree DCM CLK out
			.CLK90(),						// 90 degree DCM CLK output
			.CLKDV(),						// Divided DCM CLK out (CLKDV_DIVIDE)
			.CLKFX(),						// DCM CLK synthesis out (M/D)
			.CLKFX180(),					// 180 degree CLK synthesis out
			.LOCKED(),						// DCM LOCK status output
			.PSDONE(),						// Dynamic phase adjust done output
			.STATUS(),						// 8-bit DCM status bits output
			.CLKFB(clk),					// DCM clock feedback
			.CLKIN(clkin),					// Clock input (from IBUFG, BUFG or DCM)
			.DSSEN(1'b0),					// ?
			.PSCLK(1'b0),					// Dynamic phase adjust clock input
			.PSEN(1'b0),					// Dynamic phase adjust enable input
			.PSINCDEC(1'b0),				// Dynamic phase adjust increment/decrement
			.RST(1'b0)						// DCM asynchronous reset input
		);

	// Visible clock diagnostics on LED
	clk_diag //#(.period(16))
		uclkdiag(.clk(clk), .reset(reset), .out(FPGA_ACT));
		
	// delayed / stretched Reset generator
	wire [3:0] rst;					// Main POR delay
	FDCE #(.INIT(1'b0)) rst_bit0(.Q(rst[0]), .C(clk),.CE(1'b1), .CLR(1'b0), .D(1'b1));
	FDCE #(.INIT(1'b0)) rst_bit1(.Q(rst[1]), .C(clk),.CE(1'b1), .CLR(1'b0), .D(rst[0]));
	FDCE #(.INIT(1'b0)) rst_bit2(.Q(rst[2]), .C(clk),.CE(1'b1), .CLR(1'b0), .D(rst[1]));
	FDCE #(.INIT(1'b1)) rst_bit3(.Q(rst[3]), .C(clk),.CE(1'b1), .CLR(1'b0), .D(~rst[2]));
	assign reset = rst[3];
	
	// Asynchronous SPI MUX
	wire local_spi_miso;
	wire local_spi_clk = ~FPGA_SPI_ACK_L ? 0 : FPGA_SCLK;
	wire local_spi_mosi = ~FPGA_SPI_ACK_L ? 0 : FPGA_MOSI;
	wire local_spi_csl = ~FPGA_SPI_ACK_L ? 1 : FPGA_CSL;
	assign FPGA_MISO = ~FPGA_SPI_ACK_L ? FLASH_SO : local_spi_miso;
	
	// SPI slave port
	wire [31:0] wdat;
	reg [31:0] rdat;
	wire [6:0] addr;
	wire re, we;
	spi_slave
		uspi(.clk(clk), .reset(reset),
			.spiclk(local_spi_clk), .spimosi(local_spi_mosi),
			.spimiso(local_spi_miso), .spicsl(local_spi_csl),
			.we(we), .re(re), .wdat(wdat), .addr(addr), .rdat(rdat));
	
	// Writeable registers
	reg [31:0] freq;
	reg [9:0] wave0, wave1, wave2;
	reg [15:0] blend01;
	reg wp_n;
	always @(posedge clk)
		if(reset)
		begin
			freq <= 32'd0;
			wave0 <= 10'd0;
			wave1 <= 10'd0;
			wave2 <= 10'd0;
			blend01 <= 16'd0;
			wp_n <= 1'b0;
		end
		else if(we)
			case(addr)
				7'h01: freq <= wdat;
				7'h02: wave0 <= wdat[9:0];
				7'h03: wave1 <= wdat[9:0];
				7'h04: wave2 <= wdat[9:0];
				7'h05: blend01 <= wdat[15:0];
				7'h06: wp_n <= wdat[0];
			endcase
	
	// readback
	always @(*)
		case(addr)
			7'h00: rdat = DESIGN_ID;
			7'h01: rdat = freq;
			7'h02: rdat = {22'd0,wave0};
			7'h03: rdat = {22'd0,wave1};
			7'h04: rdat = {22'd0,wave2};
			7'h05: rdat = {16'd0,blend01};
			7'h06: rdat = {31'd0,wp_n};
			default: rdat = 32'd0;
		endcase
		
	// Flash SPI interface
	wire ena;				// sample rate enable
	wire [1:0] cyc_num;		// access cycle (0,1,2 - 3 unused)
	wire [23:0] f_addr;		// Flash address {0,page[12:0],0,byte[8:0]}
	wire [15:0] f_data;		// read data {low_byte,high_byte}
	wire f_data_stb;		// data valid strobe
	flashspi
		uut(.clk(clk), .reset(reset),
			.wrt_req_l(FPGA_SPI_REQ_L), .wrt_ack_l(FPGA_SPI_ACK_L),
			.samp_ena(ena), 
			.cyc_num(cyc_num), .addr(f_addr),
			.data(f_data), .data_stb(f_data_stb),
			.wrt_mosi(FPGA_MOSI),
			.wrt_clk(FPGA_SCLK), .wrt_cs(FPGA_CSL),
			.flsh_mosi(FLASH_SI), .flsh_miso(FLASH_SO),
			.flsh_clk(FLASH_SCK), .flsh_cs(FLASH_CS));

	// Audio Data Source
	wire signed [23:0] l_data, r_data;
	wavegen
		uwg(.clk(clk), .reset(reset), .ena(ena),
			.freq(freq),
			.wave0(wave0), .wave1(wave1), .wave2(wave2),
			.blend01(blend01),
			.cyc_num(cyc_num), .addr(f_addr),
			.data(f_data), .data_stb(f_data_stb),
			.l_data(l_data), .r_data(r_data));

	// I2S serializer - generates enable to source
	i2s_out
		ui2s(.clk(clk), .reset(reset),
			.l_data(l_data), .r_data(r_data),
			.sdout(DAC_SDIN), .sclk(DAC_SCLK),
			.lrclk(DAC_LRCK), .mclk(DAC_MCLK),
			.load(ena));
	
	// Assign Aux outputs
	assign AUX1 = {DAC_SDIN,DAC_SCLK,DAC_LRCK,DAC_MCLK};
	assign AUX2 = 4'h0;
	assign AUX3 = 4'h0;
	
	// IRQ not used yet
	assign FPGA_IRQ = 1'b0;
	
	// OOB Flash signals
	assign FLASH_WP = wp_n & ~FPGA_SPI_ACK_L; // only write when MCU and FPGA agree
	assign FLASH_RST = ~reset;
	

//`define USE_CHIPSCOPE
`ifdef USE_CHIPSCOPE
	// Chipscope stuff
	wire [35 : 0] CONTROL0;
	chipscope_icon_v1_03_a
		uicon(.CONTROL0(CONTROL0));
	chipscope_ila_v1_02_a
		uila(.CLK(clk2), .CONTROL(CONTROL0),
			.TRIG0({FLASH_SO,FLASH_SCK,FLASH_SI,FLASH_CS}));
`endif
endmodule
