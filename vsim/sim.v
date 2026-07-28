`timescale 1ns / 1ps
//============================================================================
//  Simulation top (Verilator) for the Basic Master Jr. core.
//
//  This stands in for BasicMasterJr.sv: same wiring, but hps_io and the Altera
//  PLL are replaced by things sim_main.cpp can drive.
//  Keep it in sync with BasicMasterJr.sv when the real top changes.
//
//  Deliberate differences from BasicMasterJr.sv, all noted inline:
//    * clk_vid is made with a phase accumulator instead of a PLL
//    * the 64 KiB tape RAM uses the behavioural dpram wrapper
//============================================================================

module emu (
	// 50 MHz system clock, toggled by sim_main.cpp
	input             clk_sys,
	input             reset,

	// PS/2 keyboard: [7:0] code, [8] extended, [9] pressed, [10] toggles per event
	input      [10:0] ps2_key,

	// clk_sys ticks per tape bit; --tape-bit-period sweeps this from sim_main
	input      [31:0] tape_bit_period,
	input             fast_tape,

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
	output            dbg_record_wr,
	output     [15:0] dbg_record_addr,
	output      [7:0] dbg_record_data,
	output     [16:0] dbg_record_size,
	output            dbg_record_dirty,
	output            dbg_record_overflow,
	output      [7:0] dbg_screen_mode,
	output      [7:0] dbg_kb_col,
	output      [3:0] dbg_kb_key,
	output      [3:0] dbg_kb_mod,
	output            dbg_kb_status,
	output      [3:0] dbg_kb_row0
);

//////////////////////////////  CLOCKS  //////////////////////////////////////
// The simulator uses a 25 MHz system timebase instead of the hardware's
// 50 MHz. This halves the number of Verilator evaluations per emulated second
// while leaving enough bandwidth for video.v's six-cycle-per-pixel renderer
// (17.4 MHz minimum at 59 Hz). CPU, video, tape and watchdog rates are scaled
// to preserve real-machine time.
//
// 25 / 10.638297 is not an integer, so instead of a divider we run a phase
// accumulator: clk_vid toggles when the accumulator wraps. Worst-case jitter
// is one clk_sys period (40 ns) against a 94 ns half period, which video.v
// (a free-running counter) does not care about, and it keeps the real ~59 Hz
// frame rate so screenshot frame numbers mean the same thing as on hardware.
// One accumulator carry = one clk_vid EDGE, so the increment carries twice per
// clk_vid cycle. Getting that factor of 2 wrong ran the whole video side (and
// therefore every --screenshot / --send-keys frame number) at half speed.
localparam integer SIM_CLK_HZ = 25_000_000;
localparam [31:0] VID_INC = 32'd3_655_291_016;  // round(2 * 10.638297e6 / 25e6 * 2^32)
                                                // -> 10.6383 MHz, 59.02 Hz frames

reg [31:0] vid_acc = 32'd0;
reg        clk_vid = 1'b0;

always @(posedge clk_sys) begin
	{vid_acc[31:0]} <= vid_acc + VID_INC;
	if (vid_acc + VID_INC < vid_acc) clk_vid <= ~clk_vid;  // carry out => toggle
end

// CPU clock enable and default-on fast tape loading, matching BasicMasterJr.sv.
wire tape_motor;
wire [31:0] cpu_increment = tape_motor && fast_tape
	? (32'd128_849_019 << 4)
	: 32'd128_849_019;
reg [31:0] cpu_acc = 32'd0;
wire [32:0] cpu_sum = {1'b0, cpu_acc} + {1'b0, cpu_increment};
wire clk_cpu = cpu_sum[32];

always @(posedge clk_sys)
	cpu_acc <= cpu_sum[31:0];

assign dbg_cpu_ce = clk_cpu;

wire core_reset = reset | ioctl_download;

//////////////////////////////  TAPE BUFFER  ///////////////////////////////////
// Match the hardware top: HPS downloads into a dedicated 64 KiB dual-port
// block RAM, and the cassette reads its second port. There is no external
// SDRAM handshake in the tape path, so downloads can accept one byte per tick.
wire [24:0] tape_addr;
wire  [7:0] tape_buffer_data;
wire        tape_write;
wire        tape_out;
wire        recorder_wr;
wire [15:0] recorder_addr;
wire  [7:0] recorder_data;
wire [16:0] recorder_size;
wire        recorder_dirty;
wire        recorder_overflow;

cassette_recorder recorder (
	.clk_sys(clk_sys), .reset(core_reset), .clear(1'b0), .saved(1'b0),
	.cpu_tick(clk_cpu), .motor(tape_motor), .write_strobe(tape_write),
	.buffer_wr(recorder_wr), .buffer_addr(recorder_addr),
	.buffer_data(recorder_data), .size(recorder_size),
	.dirty(recorder_dirty), .overflow(recorder_overflow)
);

wire [15:0] tape_write_addr = recorder_wr ? recorder_addr : ioctl_addr[15:0];
wire  [7:0] tape_write_data = recorder_wr ? recorder_data : ioctl_dout;

ram tape_buffer (
	.clk    (clk_sys),
	.addr   (tape_write_addr),
	.din    (tape_write_data),
	.dout   (),
	.wr     (recorder_wr | (ioctl_download & ioctl_wr & (ioctl_index == 8'd1))),
	.ce     (1'b1),
	.addr_b (tape_addr[15:0]),
	.dout_b (tape_buffer_data)
);

assign ioctl_wait = 1'b0;

//////////////////////////////  CORE  ////////////////////////////////////////
wire        hs, vs, hb, vb;
wire  [7:0] red, green, blue;
wire  [3:0] kb_key, kb_mod;
wire  [3:0] kb_col;
wire        key_pressed;
wire        ce_pix;
wire        tape_bit;
wire        tape_start;
wire        tape_stop;
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
	if (recorder_wr)
		tape_size <= {8'd0, recorder_size};
	if (old_download & ~ioctl_download)
		tape_rewind <= 1'b1;
end

cassette cassette (
	.clk_sys     (clk_sys),
	.bit_period  (tape_motor && fast_tape
	              ? ((tape_bit_period + 32'd8) >> 4)
	              : tape_bit_period),
	.reset       (reset),           // NOT core_reset: a tape mount must not rewind twice
	.tape_start  (tape_start),
	.tape_stop   (tape_stop),
	.tape_rewind (tape_rewind),
	.image_size  (tape_size),
	.sdram_addr  (tape_addr),
	.sdram_data  ({8'd0, tape_buffer_data}),
	.sdram_rd    (),
	.sdram_we    (),
	.sdram_wtbt  (),
	.sdram_ready (1'b1),
	.tape_bit    (tape_bit),
	.motor       (tape_motor),
	.status      ()
);

core #(.VIDEO_BORDER(0), .SYS_CLK_HZ(SIM_CLK_HZ)) core (
	.reset      (core_reset),
	.clk_sys    (clk_sys),
	.clk_cpu    (clk_cpu),
	.clk_vid    (clk_vid),
	.kb_col     (kb_col),
	.kb_key     (kb_key),
	.kb_mod     (kb_mod),
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
	.tape_write (tape_write),
	.tape_out   (tape_out),
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
assign dbg_record_wr       = recorder_wr;
assign dbg_record_addr     = recorder_addr;
assign dbg_record_data     = recorder_data;
assign dbg_record_size     = recorder_size;
assign dbg_record_dirty    = recorder_dirty;
assign dbg_record_overflow = recorder_overflow;
assign dbg_screen_mode = core.screen_mode;
assign dbg_kb_col     = {4'd0, kb_col};
assign dbg_kb_key     = kb_key;
assign dbg_kb_mod     = kb_mod;
assign dbg_kb_status  = keyboard.status;
assign dbg_kb_row0    = keyboard.keys[0];

endmodule
