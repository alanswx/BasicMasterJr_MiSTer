// Simulation replacement for rtl/dpram.v (the Quartus altsyncram wrapper).
// Same port list, same behaviour: 64K x 8 true dual-port, registered read,
// read-during-write on the same port returns OLD data (altsyncram default for
// this megafunction instance).
//
// This file shadows ../rtl/dpram.v -- the Makefile lists this one and not that
// one. Keep the ports in sync if the Quartus wrapper is ever regenerated.

module dpram (
	input  [15:0] address_a,
	input  [15:0] address_b,
	input         clock,
	input   [7:0] data_a,
	input   [7:0] data_b,
	input         rden_a,
	input         rden_b,
	input         wren_a,
	input         wren_b,
	output  [7:0] q_a,
	output  [7:0] q_b
);

// public_flat_rd lets sim_main.cpp read the machine's RAM for --dump-mem
reg [7:0] mem [0:65535] /*verilator public_flat_rd*/;
reg [7:0] qa_r, qb_r;

// Power-on contents. Real DRAM comes up as garbage; zero keeps sim runs
// reproducible (and matches how the FPGA block RAM initialises).
integer i;
initial begin
	for (i = 0; i < 65536; i = i + 1) mem[i] = 8'h00;
	qa_r = 8'h00;
	qb_r = 8'h00;
end

always @(posedge clock) begin
	if (wren_a) mem[address_a] <= data_a;
	if (rden_a) qa_r <= mem[address_a];
end

always @(posedge clock) begin
	if (wren_b) mem[address_b] <= data_b;
	if (rden_b) qb_r <= mem[address_b];
end

assign q_a = qa_r;
assign q_b = qb_r;

endmodule
