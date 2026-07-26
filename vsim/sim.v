`timescale 1ns / 1ps
//============================================================================
//  Simulation top (Verilator) for the Basic Master Jr. core.
//
//  This stands in for BasicMasterJr.sv: same wiring, but hps_io / the Altera
//  PLL / the SDRAM controller are replaced by things sim_main.cpp can drive.
//  Keep it in sync with BasicMasterJr.sv when the real top changes.
//
//  Deliberate differences from BasicMasterJr.sv, all noted inline:
//    * clk_vid is made with a phase accumulator instead of a PLL
//    * sdram is a behavioural byte array (vsim/rtl/sdram.sv)
//    * sdram_wtbt has a single driver here (the real top double-drives it)
//============================================================================

module emu (
	// 50 MHz system clock, toggled by sim_main.cpp
	input             clk_sys,
	input             reset,

	// PS/2 keyboard: [7:0] code, [8] extended, [9] pressed, [10] toggles per event
	input      [10:0] ps2_key,

	// clk_sys ticks per tape bit; --tape-bit-period sweeps this from sim_main
	input      [31:0] tape_bit_period,

	// HPS file transfer (tape images) -- driven by sim/sim_bus.cpp
	input             ioctl_download,
	input             ioctl_wr,
	input      [24:0] ioctl_addr,
	input       [7:0] ioctl_dout,
	input       [7:0] ioctl_index,
	output            ioctl_wait,

	// Video
	output      [7:0] VGA_R,
	output      [7:0] VGA_G,
	output      [7:0] VGA_B,
	output            VGA_HS,
	output            VGA_VS,
	output            VGA_HB,
	output            VGA_VB,
	output            CE_PIXEL,

	// Audio (the core has no sound yet -- see TODO.md)
	output     [15:0] AUDIO_L,
	output     [15:0] AUDIO_R,

	// --- debug taps for sim_main.cpp -----------------------------------
	// Hierarchical reads out of the core; no RTL changes required.
	output     [15:0] dbg_cpu_addr,
	output      [7:0] dbg_cpu_din,
	output      [7:0] dbg_cpu_dout,
	output            dbg_cpu_rw,
	output            dbg_cpu_vma,
	output            dbg_cpu_ce,      // 1 for one clk_sys tick per CPU cycle
	output      [2:0] dbg_bank,
	output            dbg_tape_start,
	output            dbg_tape_stop,
	output            dbg_tape_bit,
	output            dbg_tape_motor,
	output      [2:0] dbg_tape_state,
	output     [24:0] dbg_tape_addr,
	output     [24:0] dbg_tape_size,
	output      [7:0] dbg_tape_byte,
	output      [7:0] dbg_screen_mode,
	output      [7:0] dbg_kb_col,
	output      [3:0] dbg_kb_key,
	output      [3:0] dbg_kb_mod,
	output            dbg_kb_status,
	output      [3:0] dbg_kb_row0
);

//////////////////////////////  CLOCKS  //////////////////////////////////////
// Real hardware: PLL gives clk_sys = 50 MHz and clk_vid = 10.638297 MHz.
// 50 / 10.638297 is not an integer, so instead of a divider we run a phase
// accumulator: clk_vid toggles when the accumulator wraps. Worst-case jitter
// is one clk_sys period (20 ns) against a 94 ns half period, which video.v
// (a free-running counter) does not care about, and it keeps the real ~59 Hz
// frame rate so screenshot frame numbers mean the same thing as on hardware.
// One accumulator carry = one clk_vid EDGE, so the increment carries twice per
// clk_vid cycle. Getting that factor of 2 wrong ran the whole video side (and
// therefore every --screenshot / --send-keys frame number) at half speed.
localparam [31:0] VID_INC = 32'd1_827_645_508;  // round(2 * 10.638297e6 / 50e6 * 2^32)
                                                // -> 10.6383 MHz, 59.02 Hz frames

reg [31:0] vid_acc = 32'd0;
reg        clk_vid = 1'b0;

always @(posedge clk_sys) begin
	{vid_acc[31:0]} <= vid_acc + VID_INC;
	if (vid_acc + VID_INC < vid_acc) clk_vid <= ~clk_vid;  // carry out => toggle
end

// CPU clock -- identical to BasicMasterJr.sv. core.v uses clk_cpu as a real
// clock (posedge clk_cpu), so this is a pulse train, not an enable.
// 3 CPU cycles = 200 clk_sys ticks => exactly 750,000 Hz on average.
reg [6:0] div = 7'd0;
reg [1:0] phase = 2'd0;
wire [6:0] div_last = (phase == 2'd2) ? 7'd65 : 7'd66;
wire clk_cpu = (div == 7'd0);

always @(posedge clk_sys) begin
	if (div < div_last) div <= div + 7'd1;
	else begin
		div <= 7'd0;
		phase <= (phase == 2'd2) ? 2'd0 : phase + 2'd1;
	end
end

assign dbg_cpu_ce = clk_cpu;

wire core_reset = reset | ioctl_download;

//////////////////////////////  SDRAM  ///////////////////////////////////////
wire [24:0] sdram_addr;
wire [15:0] sdram_data;
wire        sdram_rd;
wire        sdram_we;
wire  [1:0] sdram_wtbt;
wire        sdram_ready;

// Behavioural stand-in for rtl/sdram.sv (vsim/rtl/sdram.sv). Byte addressed,
// same ready/rd handshake, no chip pins.
// init must exclude the download: it forces the controller back to
// STATE_STARTUP every cycle it is high, which discards the transfer.
sdram sdram_inst (
	.init  (reset),
	.clk   (clk_sys),
	.wtbt  (ioctl_download ? 2'b00 : sdram_wtbt),
	.addr  (ioctl_download ? ioctl_addr : sdram_addr),
	.dout  (sdram_data),
	.din   ({8'd0, ioctl_dout}),
	.we    (ioctl_download & ioctl_wr),
	.rd    (sdram_rd),
	.ready (sdram_ready)
);

// The real top leaves ioctl_wait undriven. Drive it from the SDRAM handshake
// so the sim never silently drops bytes out of a tape image; if you are
// chasing a download bug, tie it to 0 to reproduce hardware behaviour.
assign ioctl_wait = ioctl_download & ~sdram_ready;

//////////////////////////////  CORE  ////////////////////////////////////////
wire        hs, vs, hb, vb;
wire  [7:0] red, green, blue;
wire  [3:0] kb_key, kb_mod;
wire  [7:0] kb_col;
wire        key_pressed;
wire        ce_pix;
wire        tape_bit;
wire        tape_start;
wire        tape_stop;
wire        tape_motor;
wire        break_key;
wire  [4:0] dac_level;

// Tape image size + rewind on mount, mirroring BasicMasterJr.sv.
reg [24:0] tape_size;
reg        tape_rewind;
always @(posedge clk_sys) begin
	reg old_download;
	old_download <= ioctl_download;
	tape_rewind <= 1'b0;
	if (ioctl_download & ioctl_wr & (ioctl_index == 8'd1))
		tape_size <= ioctl_addr + 25'd1;
	if (old_download & ~ioctl_download)
		tape_rewind <= 1'b1;
end

cassette cassette (
	.clk_sys     (clk_sys),
	.bit_period  (tape_bit_period),
	.reset       (reset),           // NOT core_reset: a tape mount must not rewind twice
	.tape_start  (tape_start),
	.tape_stop   (tape_stop),
	.tape_rewind (tape_rewind),
	.image_size  (tape_size),
	.sdram_addr  (sdram_addr),
	.sdram_data  (sdram_data),
	.sdram_rd    (sdram_rd),
	.sdram_we    (sdram_we),
	.sdram_wtbt  (sdram_wtbt),
	.sdram_ready (sdram_ready),
	.tape_bit    (tape_bit),
	.motor       (tape_motor),
	.status      ()
);

core core (
	.reset      (core_reset),
	.clk_sys    (clk_sys),
	.clk_cpu    (clk_cpu),
	.clk_vid    (clk_vid),
	.kb_col     (kb_col),
	.kb_key     (kb_key),
	.kb_mod     (kb_mod),
	.key_pressed(key_pressed),
	.hs         (hs),
	.vs         (vs),
	.hb         (hb),
	.vb         (vb),
	.ce_pix     (ce_pix),
	.red        (red),
	.green      (green),
	.blue       (blue),
	.break_key  (break_key),
	.tape_bit   (tape_bit),
	.tape_start (tape_start),
	.tape_stop  (tape_stop),
	.dac_level  (dac_level)
);

keyboard keyboard (
	.reset  (reset),
	.clk_sys(clk_sys),
	.ps2_key(ps2_key),
	.kb_col (kb_col),
	.kb_key (kb_key),
	.kb_mod (kb_mod),
	.break_key(break_key),
	.status (key_pressed)
);

//////////////////////////////  OUTPUTS  /////////////////////////////////////
// R/G are crossed on purpose -- see BasicMasterJr.sv. video.v emits
// {R,G,B} = {bit2,bit1,bit0} of the pen; the machine's pen order is BRG
// (R=bit1, G=bit2, B=bit0), and this swap turns one into the other.
assign VGA_R    = green;
assign VGA_G    = red;
assign VGA_B    = blue;
assign VGA_HS   = hs;
assign VGA_VS   = vs;
assign VGA_HB   = hb;
assign VGA_VB   = vb;
assign CE_PIXEL = ce_pix;

assign AUDIO_L = {dac_level, 11'd0};
assign AUDIO_R = {dac_level, 11'd0};

//////////////////////////////  DEBUG TAPS  //////////////////////////////////
assign dbg_cpu_addr   = core.cpu_addr;
assign dbg_cpu_din    = core.cpu_din;
assign dbg_cpu_dout   = core.cpu_dout;
assign dbg_cpu_rw     = core.cpu_rw;
assign dbg_cpu_vma    = core.vma;
assign dbg_bank       = core.bank;
assign dbg_tape_start = tape_start;
assign dbg_tape_stop  = tape_stop;
assign dbg_tape_bit   = tape_bit;
assign dbg_tape_motor = tape_motor;
assign dbg_tape_state = cassette.state;
assign dbg_tape_addr  = cassette.sdram_addr;
assign dbg_tape_size  = tape_size;
assign dbg_tape_byte  = cassette.shifter;
assign dbg_screen_mode = core.screen_mode;
assign dbg_kb_col     = kb_col;
assign dbg_kb_key     = kb_key;
assign dbg_kb_mod     = kb_mod;
assign dbg_kb_status  = keyboard.status;
assign dbg_kb_row0    = keyboard.keys[0];

endmodule
