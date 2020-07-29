// flashspi.v: SPI interface for AT45DB321D
// 2010-02-07 E. Brombaugh

module flashspi(clk, reset, wrt_req_l, wrt_ack_l, samp_ena, 
		cyc_num, addr, data, data_stb,
		wrt_mosi, wrt_clk, wrt_cs,
		flsh_mosi, flsh_miso, flsh_clk, flsh_cs);
	parameter asz = 24;			// Address bus size
	parameter dsz = 16;			// Data bus size

	input clk;					// 24.576MHz system clock
	input reset;				// POR
	input wrt_req_l;			// Request write mux override
	output wrt_ack_l;			// Acknowledge write mux request
	input samp_ena;				// Sample rate enable (1/256)
	output [1:0] cyc_num;		// Cycle number (0-2, 3 unused)
	input [asz-1:0] addr;		// Flash Read Address
	output [dsz-1:0] data;		// Flash Read Data
	output data_stb;			// Data Valid strobe
	input wrt_mosi;				// Write SPI MOSI override
	input wrt_clk;				// Write SPI Clock override
	input wrt_cs;				// Write SPI CS override
	output flsh_mosi;			// Flash SPI MOSI
	input flsh_miso;			// Flash SPI MISO
	output flsh_clk;			// Flash SPI Clock
	output flsh_cs;				// Flash SPI CS

	// MUX access control
	reg wrt_mux;
	always @(posedge clk)
		if(reset)
			wrt_mux <= 1'b0;
		else if(cyc_num == 2'b11)
			wrt_mux <= ~wrt_req_l;
		
	// Top-level cycle tracker
	reg [1:0] cyc_num, next_cyc_num;
	reg trig, next_trig;
	reg done, next_done;
	always @(posedge clk)
		if(reset)
		begin
			cyc_num <= 2'b11;
			trig <= 1'b0;
		end
		else
		begin
			cyc_num <= next_cyc_num;
			trig <= next_trig;
		end
	always @(*)
	begin
		// Defaults
		next_cyc_num = cyc_num;
		next_trig = 1'b0;
		
		case(cyc_num)
			2'b11:	// Wait for sample enable
			begin
				if(samp_ena & ~wrt_mux)
				begin
					next_cyc_num = cyc_num + 1;
					next_trig = 1'b1;
				end
			end

			default:	// advance to next cycle when SPI complete
			begin
				if(done)
				begin
					next_cyc_num = cyc_num + 1;
					if(cyc_num != 2'b10)
						next_trig = ~wrt_mux;	// only trig if not muxed out
				end
			end
		endcase
	end
	
	// SPI sequencer
	reg [6:0] state, next_state;
	reg cs, next_cs;
	always @(posedge clk)
		if(reset)
		begin
			state <= 7'h00;
			cs <= 1'b1;
			done <= 1'b0;
		end
		else
		begin
			state <= next_state;
			cs <= next_cs;
			done <= next_done;
		end
	always @(*)
	begin
		// defaults
		next_state = state;
		next_cs = cs;
		next_done = 0;
		
		case(state)
			7'h00:	// Wait for trigger from cycle counter
			begin
				if(trig)
				begin
					next_state = state + 1;
					next_cs = ~(cyc_num != 2'b11);
				end
			end

			7'h40:	// Finished - back to start
			begin
				next_state = 7'h00;
				next_cs = 1'b1;
				next_done = 1'b1;
			end

			default:	// advance to next state unless Sample restarts us
				if(samp_ena)
				begin
					next_state = 7'h00;
					//next_done = 1'b1;
				end
				else
					next_state = state + 1;
		endcase
	end
	
	// Decode 4 cycles when to grab cmd/addr
	reg p_addr_stb;
	always @(*)
		case(state)
			7'h00: p_addr_stb = trig;
			7'h08: p_addr_stb = ~cs;
			7'h10: p_addr_stb = ~cs;
			7'h18: p_addr_stb = ~cs;
			default: p_addr_stb = 1'b0;
		endcase
	
	// mux cmd/address
	reg [7:0] p_dout;
	always @(*)
		case(state[4:3])
			2'h0: p_dout = 8'h03;
			2'h1: p_dout = addr[23:16];
			2'h2: p_dout = addr[15:8];
			2'h3: p_dout = addr[7:0];
		endcase
	
	// SPI output shift register loads with command/address at start
	reg [7:0] dout;
	always @(posedge clk)
		if(p_addr_stb)
			dout <= p_dout;
		else
			dout <= {dout[6:0],1'b0};
	
	// Decode 2 cycles when to grab input data
	reg p_data_stb, data_stb;
	always @(*)
		case(state)
			7'h30: p_data_stb = ~cs;
			7'h40: p_data_stb = ~cs;
			default: p_data_stb = 1'b0;
		endcase
	always @(posedge clk)
		data_stb <= p_data_stb;
	
	// SPI input shift register loads whenever CS active
	reg [dsz-2:0] din;
	always @(posedge clk)
		if(~cs)
			din <= {din[14:0],flsh_miso};
	
	// 16-bit Data hold register
	reg [dsz-1:0] data;
	always @(posedge clk)
		if(p_data_stb)
			data <= {din[14:0],flsh_miso};
		
	// mux access to Flash SPI outputs
	assign flsh_mosi = wrt_mux ? wrt_mosi : dout[7];
	assign flsh_clk = wrt_mux ? wrt_clk : ~clk;
	assign flsh_cs = wrt_mux ? wrt_cs : cs;
	assign wrt_ack_l = ~wrt_mux;
endmodule

