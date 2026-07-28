//============================================================================
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//============================================================================

module emu
(
	//Master input clock
	input         CLK_50M,

	//Async reset from top-level module.
	//Can be used as initial reset.
	input         RESET,

	//Must be passed to hps_io module
	inout  [48:0] HPS_BUS,

	//Base video clock. Usually equals to CLK_SYS.
	output        CLK_VIDEO,

	//Multiple resolutions are supported using different CE_PIXEL rates.
	//Must be based on CLK_VIDEO
	output        CE_PIXEL,

	//Video aspect ratio for HDMI. Most retro systems have ratio 4:3.
	//if VIDEO_ARX[12] or VIDEO_ARY[12] is set then [11:0] contains scaled size instead of aspect ratio.
	output [12:0] VIDEO_ARX,
	output [12:0] VIDEO_ARY,

	output  [7:0] VGA_R,
	output  [7:0] VGA_G,
	output  [7:0] VGA_B,
	output        VGA_HS,
	output        VGA_VS,
	output        VGA_DE,    // = ~(VBlank | HBlank)
	output        VGA_F1,
	output [1:0]  VGA_SL,
	output        VGA_SCALER, // Force VGA scaler

	input  [11:0] HDMI_WIDTH,
	input  [11:0] HDMI_HEIGHT,
	output        HDMI_FREEZE,

`ifdef MISTER_FB
	// Use framebuffer in DDRAM (USE_FB=1 in qsf)
	// FB_FORMAT:
	//    [2:0] : 011=8bpp(palette) 100=16bpp 101=24bpp 110=32bpp
	//    [3]   : 0=16bits 565 1=16bits 1555
	//    [4]   : 0=RGB  1=BGR (for 16/24/32 modes)
	//
	// FB_STRIDE either 0 (rounded to 256 bytes) or multiple of pixel size (in bytes)
	output        FB_EN,
	output  [4:0] FB_FORMAT,
	output [11:0] FB_WIDTH,
	output [11:0] FB_HEIGHT,
	output [31:0] FB_BASE,
	output [13:0] FB_STRIDE,
	input         FB_VBL,
	input         FB_LL,
	output        FB_FORCE_BLANK,

`ifdef MISTER_FB_PALETTE
	// Palette control for 8bit modes.
	// Ignored for other video modes.
	output        FB_PAL_CLK,
	output  [7:0] FB_PAL_ADDR,
	output [23:0] FB_PAL_DOUT,
	input  [23:0] FB_PAL_DIN,
	output        FB_PAL_WR,
`endif
`endif

	output        LED_USER,  // 1 - ON, 0 - OFF.

	// b[1]: 0 - LED status is system status OR'd with b[0]
	//       1 - LED status is controled solely by b[0]
	// hint: supply 2'b00 to let the system control the LED.
	output  [1:0] LED_POWER,
	output  [1:0] LED_DISK,

	// I/O board button press simulation (active high)
	// b[1]: user button
	// b[0]: osd button
	output  [1:0] BUTTONS,

	input         CLK_AUDIO, // 24.576 MHz
	output [15:0] AUDIO_L,
	output [15:0] AUDIO_R,
	output        AUDIO_S,   // 1 - signed audio samples, 0 - unsigned
	output  [1:0] AUDIO_MIX, // 0 - no mix, 1 - 25%, 2 - 50%, 3 - 100% (mono)

	//ADC
	inout   [3:0] ADC_BUS,

	//SD-SPI
	output        SD_SCK,
	output        SD_MOSI,
	input         SD_MISO,
	output        SD_CS,
	input         SD_CD,

	//High latency DDR3 RAM interface
	//Use for non-critical time purposes
	output        DDRAM_CLK,
	input         DDRAM_BUSY,
	output  [7:0] DDRAM_BURSTCNT,
	output [28:0] DDRAM_ADDR,
	input  [63:0] DDRAM_DOUT,
	input         DDRAM_DOUT_READY,
	output        DDRAM_RD,
	output [63:0] DDRAM_DIN,
	output  [7:0] DDRAM_BE,
	output        DDRAM_WE,

	//SDRAM interface with lower latency
	output        SDRAM_CLK,
	output        SDRAM_CKE,
	output [12:0] SDRAM_A,
	output  [1:0] SDRAM_BA,
	inout  [15:0] SDRAM_DQ,
	output        SDRAM_DQML,
	output        SDRAM_DQMH,
	output        SDRAM_nCS,
	output        SDRAM_nCAS,
	output        SDRAM_nRAS,
	output        SDRAM_nWE,

`ifdef MISTER_DUAL_SDRAM
	//Secondary SDRAM
	//Set all output SDRAM_* signals to Z ASAP if SDRAM2_EN is 0
	input         SDRAM2_EN,
	output        SDRAM2_CLK,
	output [12:0] SDRAM2_A,
	output  [1:0] SDRAM2_BA,
	inout  [15:0] SDRAM2_DQ,
	output        SDRAM2_nCS,
	output        SDRAM2_nCAS,
	output        SDRAM2_nRAS,
	output        SDRAM2_nWE,
`endif

	input         UART_CTS,
	output        UART_RTS,
	input         UART_RXD,
	output        UART_TXD,
	output        UART_DTR,
	input         UART_DSR,

	// Open-drain User port.
	// 0 - D+/RX
	// 1 - D-/TX
	// 2..6 - USR2..USR6
	// Set USER_OUT to 1 to read from USER_IN.
	input   [6:0] USER_IN,
	output  [6:0] USER_OUT,

	input         OSD_STATUS
);

///////// Default values for ports not used in this core /////////

//assign ADC_BUS  = 'Z;
assign USER_OUT = '1;
assign {UART_RTS, UART_TXD, UART_DTR} = 0;
assign {SD_SCK, SD_MOSI, SD_CS} = 'Z;
assign SDRAM_CLK = clk_sys;
assign {DDRAM_CLK, DDRAM_BURSTCNT, DDRAM_ADDR, DDRAM_DIN, DDRAM_BE, DDRAM_RD, DDRAM_WE} = '0;

assign VGA_F1 = 0;
assign VGA_SCALER = 0;
assign HDMI_FREEZE = 0;

// $EE80 bits 5:1 drive a 5-bit R-2R ladder into the speaker amp (IC40 +
// R36-R45 on the schematic). Unsigned, mono, same level on both channels.
assign AUDIO_S = 0;
assign AUDIO_L = {dac_level, 11'd0};
assign AUDIO_R = {dac_level, 11'd0};
assign AUDIO_MIX = 0;

assign LED_USER = 0;
assign LED_DISK = 0;
assign LED_POWER = 0;
assign BUTTONS = 0;

//////////////////////////////////////////////////////////////////

wire [1:0] ar = status[122:121];

assign VIDEO_ARX = (!ar) ? 12'd4 : (ar - 1'd1);
assign VIDEO_ARY = (!ar) ? 12'd3 : 12'd0;

`include "build_id.v"
localparam CONF_STR = {
	"BasicMasterJr;;",
	"-;",
	// The extension field is 3-character extensions concatenated, so "Tape"
	// was malformed (read as "Tap" plus a stray "e"). Tape images are .bin.
	"F1,BIN,Load Tape;",
	"-;",
	"O[118],Fast tape loading,On,Off;",
	"O[117:115],Scandoubler Fx,None,HQ2x,CRT 25%,CRT 50%,CRT 75%;",
	"-;",
	"O[122:121],Aspect ratio,Original,Full Screen,[ARC1],[ARC2];",
	"-;",
	"T[0],Reset;",
	"R[0],Reset and close OSD;",
	"V,v",`BUILD_DATE
};

wire forced_scandoubler;
wire [21:0] gamma_bus;
wire   [1:0] buttons;
wire [127:0] status;
wire  [10:0] ps2_key;

wire [24:0] ioctl_addr;
wire  [7:0] ioctl_dout;
wire  [7:0] ioctl_index;
wire ioctl_wr;
wire ioctl_download;
wire ioctl_wait;

hps_io #(.CONF_STR(CONF_STR)) hps_io
(
	.clk_sys(clk_sys),
	.HPS_BUS(HPS_BUS),
	.EXT_BUS(),
	.gamma_bus(gamma_bus),

	.forced_scandoubler(forced_scandoubler),

	.buttons(buttons),
	.status(status),
	.status_menumask({status[5]}),

	.ps2_key(ps2_key),

	.ioctl_download(ioctl_download),
	.ioctl_wr(ioctl_wr),
	.ioctl_addr(ioctl_addr),
	.ioctl_dout(ioctl_dout),
	.ioctl_wait(ioctl_wait),
	.ioctl_index(ioctl_index)
);

///////////////////////   CLOCKS   ///////////////////////////////

wire clk_sys, clk_vid;
pll pll
(
	.refclk(CLK_50M),
	.rst(0),
	.outclk_0(clk_sys),
	.outclk_1(clk_vid) // 25 MHz video_mixer master; 4 clocks per pixel
);


// HD46800 CPU clock enable. The base increment is
// round(750 kHz / 50 MHz * 2^32). With fast tape loading enabled (the default),
// the cassette motor multiplies both this enable rate and the tape baud rate by
// 16, preserving the monitor ROM's 2500 CPU cycles per tape bit.
wire tape_motor;
wire fast_tape_loading = ~status[118];
wire [31:0] cpu_increment = tape_motor && fast_tape_loading
                          ? (32'd64_424_509 << 4)
                          : 32'd64_424_509;
reg [31:0] cpu_acc = 32'd0;
wire [32:0] cpu_sum = {1'b0, cpu_acc} + {1'b0, cpu_increment};
wire clk_cpu = cpu_sum[32];

always @(posedge clk_sys)
  cpu_acc <= cpu_sum[31:0];

wire reset = RESET | status[0] | buttons[1] | ioctl_download;

// The SDRAM controller must NOT be re-initialised while a tape image is being
// written into it: `init` forces its state machine back to STATE_STARTUP every
// cycle it is high, so every byte of the transfer was being thrown away.
wire sdram_init = RESET | status[0] | buttons[1];

//////////////////////////////////////////////////////////////////

wire hs, vs, hb, vb;
wire [7:0] red, green, blue;
wire [3:0] kb_key, kb_mod;
wire [3:0] kb_col;

wire adc_cassette_bit;
wire ce_pix;
wire break_key;
wire [4:0] dac_level;

// The cassette reads the on-chip tape buffer. Keep the physical SDRAM
// controller initialized and idle so its external pins remain well defined.
wire [24:0] cassette_sdram_addr;
wire tape_start;
wire tape_stop;
wire [31:0] tape_bit_period =
  tape_motor && fast_tape_loading ? 32'd10417 : 32'd166667;

// The tape image lives in a dedicated dual-port 64 KiB block RAM. hps_io
// writes one port at full speed while the cassette reads the other, avoiding
// the edge-triggered external SDRAM handshake and byte-lane hazards.
assign ioctl_wait = 1'b0;

// Latch how many bytes the tape image holds, so the player knows where the
// tape ends instead of wrapping through stale buffer data. Reset the
// position whenever a new image is mounted.
reg [24:0] tape_size;
reg        tape_rewind;
wire [7:0] tape_buffer_data;
always @(posedge clk_sys) begin
  reg old_download;
  old_download <= ioctl_download;
  tape_rewind <= 1'b0;
  if (ioctl_download & ioctl_wr & (ioctl_index == 8'd1))
    tape_size <= ioctl_addr + 25'd1;
  if (old_download & ~ioctl_download)
    tape_rewind <= 1'b1;
end

sdram sdram_inst(
  .init(sdram_init),
  .clk(clk_sys),
  .wtbt(2'b00),
  .addr(25'd0),
  .dout(),
  .din(16'd0),
  .we(1'b0),
  .rd(1'b0),
  .ready(),
  .SDRAM_CKE(SDRAM_CKE),
  .SDRAM_A(SDRAM_A),
  .SDRAM_BA(SDRAM_BA),
  .SDRAM_DQ(SDRAM_DQ),
  .SDRAM_DQML(SDRAM_DQML),
  .SDRAM_DQMH(SDRAM_DQMH),
  .SDRAM_nCS(SDRAM_nCS),
  .SDRAM_nRAS(SDRAM_nRAS),
  .SDRAM_nCAS(SDRAM_nCAS),
  .SDRAM_nWE(SDRAM_nWE)
);

wire [15:0] tape_buffer_addr = cassette_sdram_addr[15:0];

ram tape_buffer(
  .clk(clk_sys),
  .addr(ioctl_addr[15:0]),
  .din(ioctl_dout),
  .dout(),
  .wr(ioctl_download & ioctl_wr & (ioctl_index == 8'd1)),
  .ce(1'b1),
  .addr_b(tape_buffer_addr),
  .dout_b(tape_buffer_data)
);

cassette cassette(
  .clk_sys(clk_sys),
  // Tape bit period: 50 MHz / 300 baud. This is the real rate -- it works
  // because rtl/m6800_pad.v makes the CPU keep 6800 time. Before that the
  // monitor's software-timed bit loop ran ~19% fast and this had to be fudged
  // to 145000. If tapes stop loading, check the CPU timing first
  // (cd vsim/cputest && make && ./obj_dir/Vcputb --pad).
  .bit_period(tape_bit_period),
  .reset(reset),
  .tape_start(tape_start),
  .tape_stop(tape_stop),
  .tape_rewind(tape_rewind),
  .image_size(tape_size),
  .sdram_addr(cassette_sdram_addr),
  .sdram_data({8'd0, tape_buffer_data}),
  .sdram_rd(),
  .sdram_we(),
  .sdram_wtbt(),
  .sdram_ready(1'b1),
  .tape_bit(adc_cassette_bit),
  .motor(tape_motor),
  .status()
);

core core(
  .reset(reset),
  .clk_sys(clk_sys),
  .clk_cpu(clk_cpu),
  .clk_vid(clk_vid),
  .kb_col(kb_col),
  .kb_key(kb_key),
  .kb_mod(kb_mod),
  .hs(hs),
  .vs(vs),
  .hb(hb),
  .vb(vb),
	.ce_pix(ce_pix),
  .red(red),
  .green(green),
  .blue(blue),
  .break_key(break_key),
  .tape_bit(adc_cassette_bit),
  .tape_start(tape_start),
  .tape_stop(tape_stop),
  .dac_level(dac_level)
);

keyboard keyboard(
  .reset(reset),
  .clk_sys(clk_sys),
  .ps2_key(ps2_key),
  .kb_col(kb_col),
  .kb_key(kb_key),
  .kb_mod(kb_mod),
  .break_key(break_key),
  .status()
);

assign CLK_VIDEO = clk_vid;

wire [2:0] video_fx = status[117:115];
wire [2:0] scanline_fx = video_fx ? video_fx - 3'd1 : 3'd0;
wire scandoubler = forced_scandoubler | (video_fx != 3'd0);
assign VGA_SL = scanline_fx[1:0];

video_mixer #(
  .LINE_LENGTH(324),
  .HALF_DEPTH(0),
  .GAMMA(1)
) video_mixer (
  .CLK_VIDEO(clk_vid),
  .ce_pix(ce_pix),
  .CE_PIXEL(CE_PIXEL),
  .scandoubler(scandoubler),
  .hq2x(video_fx == 3'd1),
  .gamma_bus(gamma_bus),
  // Preserve the board's established red/green mapping.
  .R(green),
  .G(red),
  .B(blue),
  .HSync(hs),
  .VSync(vs),
  .HBlank(hb),
  .VBlank(vb),
  .HDMI_FREEZE(1'b0),
  .freeze_sync(),
  .VGA_R(VGA_R),
  .VGA_G(VGA_G),
  .VGA_B(VGA_B),
  .VGA_VS(VGA_VS),
  .VGA_HS(VGA_HS),
  .VGA_DE(VGA_DE)
);


endmodule
