// tape_overlay.sv - tape status bar over the picture (OSD "Tape overlay").
//
// Ported from the Sharp X1 core's rtl/x1_tape_overlay.sv (GPL-2.0), the
// reference for ~/dev2/tape-standard.md. While the tape plays, the bottom-left
// corner shows
//
//     TAPE > 0:09/ 0:24 x32
//     [=========---------]
//
// elapsed/total time at the real 300 baud, the turbo factor while turbo is on,
// and a 64-step progress bar. A Jr tape image is the demodulated byte stream at
// a fixed 300 baud, 11 bit-times a byte, so the times are exact: a byte is 11
// bit-times of 1/300 s. tape_status (clk_sys) packs everything into one word
// and hands it to tape_overlay (clk_vid) with a toggle handshake. The overlay
// is mixed in after the machine and before video_mixer; it never changes the
// machine.
//
// While the deck is stopped, the same corner shows what to type next
// (tape_hint): the load command for the tape's first file, then the start
// command once the tape has played to the end. Text is drawn with the
// machine's own character ROM.

// Special character codes in hint and status text (the ROM has block graphics
// there; the overlay draws its own glyphs instead).
`define TH_RETURN 8'h01
`define TH_SPACE  8'h02
`define TH_PLAY   8'h03

// tape_hint - what to type next, read from the tape's block headers.
//
// The image is parsed as hps_io writes it: each block is a $FF leader, $01,
// a 12-byte header (kind, ascii block, 6-char name, ".B"/".S", binary block,
// size), the load address, a header checksum, `size` data bytes (0 = 256),
// a data checksum and one trailing byte (software/bmtape_info.py). The tape
// says nothing about where a program starts; every machine-code tape we have
// starts where its first block loads ($1000, $2200 or $3000), so the hint
// uses that.
//
// The first file is A. The first later header with another kind or name is B:
// Tiny Galaxy and MISSILE FIGHT are a machine-code file followed by a BASIC
// one. Files after B are ignored. With the deck stopped:
//
//   at the start of the tape     load A      TYPE MON <ret> L MAZY <spc>
//   between A and B              load B      TYPE E <ret> LOAD GALAXY <ret>
//   at the end of the tape       start it    TYPE G 1000 <spc> / TYPE RUN <ret>
//
// The start hint goes away when its last key (Space or Return) is pressed.
// After a recording the tape also sits at its end, so the start hint stays
// off until the tape is rewound (which then shows the load hint).

module tape_hint (
  input  logic         clk,
  input  logic         wr,          // tape image byte written (download or recording)
  input  logic         rec,         // ... by the recorder
  input  logic  [24:0] addr,
  input  logic   [7:0] data,
  input  logic  [24:0] position,    // cassette byte pointer
  input  logic  [24:0] length,      // bytes on the tape
  input  logic         playing,
  input  logic  [10:0] ps2_key,
  output logic         show,
  output logic   [4:0] len,
  output logic [175:0] text         // 22 characters, the first in [175:168]
);
  localparam logic [1:0] P_SEEK = 2'd0, P_HDR = 2'd1, P_SKIP = 2'd2;
  localparam int SLOTS = 45;

  logic [1:0]  pstate;
  logic [3:0]  hidx;
  logic [7:0]  hdr [0:14];
  logic [8:0]  skip;
  logic [24:0] mark;
  // File A and file B.
  logic        a_valid, a_bin, b_valid, b_bin;
  logic [47:0] a_name, b_name;
  logic [15:0] a_addr, b_addr;
  logic [24:0] a_last, b_off;     // A's last block and B's first, as $01 offsets
  logic [24:0] z_last;            // the image's last block

  wire  [7:0]  kind = hdr[0];
  wire  [47:0] hname = {hdr[2], hdr[3], hdr[4], hdr[5], hdr[6], hdr[7]};
  wire  [8:0]  size = hdr[11] != 8'd0 ? {1'b0, hdr[11]} : kind == 8'd0 ? 9'd0 : 9'd256;

  initial begin
    pstate = P_SEEK; hidx = 0; skip = 0; mark = 0;
    a_valid = 0; a_bin = 0; b_valid = 0; b_bin = 0;
    a_name = 0; b_name = 0; a_addr = 0; b_addr = 0; a_last = 0; b_off = 0; z_last = 0;
  end

  always_ff @(posedge clk) begin
    if (wr) begin
      if (addr == 25'd0) begin
        a_valid <= 0; b_valid <= 0;
        pstate <= data == 8'h01 ? P_HDR : P_SEEK;
        hidx <= 0; mark <= 0;
      end else case (pstate)
        P_SEEK: if (data == 8'h01) begin pstate <= P_HDR; hidx <= 0; mark <= addr; end
        P_HDR: begin
          hdr[hidx] <= data;
          hidx <= hidx + 4'd1;
          if (hidx == 4'd14) begin
            // hdr[14] (the header checksum) is this byte; the rest are in.
            skip <= size + 9'd2;
            pstate <= P_SKIP;
            z_last <= mark;
            if (!a_valid) begin
              if (kind != 8'd0) begin
                a_valid <= 1; a_bin <= kind == 8'h01; a_name <= hname;
                a_addr <= {hdr[12], hdr[13]}; a_last <= mark;
              end
            end else if (!b_valid) begin
              if (kind != 8'd0 && ((kind == 8'h01) != a_bin || hname != a_name)) begin
                b_valid <= 1; b_bin <= kind == 8'h01; b_name <= hname;
                b_addr <= {hdr[12], hdr[13]}; b_off <= mark;
              end else a_last <= mark;
            end
          end
        end
        P_SKIP: begin
          skip <= skip - 9'd1;
          if (skip == 9'd1) pstate <= P_SEEK;
        end
        default: pstate <= P_SEEK;
      endcase
    end
  end

  // Which hint, if any.
  wire at_start = position == 25'd0;
  // An image may carry padding after its last block, so "played" means past
  // the last header rather than at the last byte.
  wire at_end   = position >= length || position > z_last;
  wire between  = b_valid && position > a_last && position <= b_off;
  wire loaded   = length != 25'd0 && a_valid;
  wire load_a   = at_start;
  wire load_b   = !at_start && !at_end && between;
  wire run      = at_end && !at_start;
  wire run_bin  = b_valid ? b_bin : a_bin;

  // The start hint is dismissed by the key that ends it.
  logic old_strobe, dismissed, recorded;
  initial begin old_strobe = 0; dismissed = 0; recorded = 0; end
  always_ff @(posedge clk) begin
    old_strobe <= ps2_key[10];
    if (rec) recorded <= 1;
    else if (at_start) recorded <= 0;
    if (!run) dismissed <= 0;
    else if (ps2_key[10] != old_strobe && ps2_key[9] &&
             ps2_key[7:0] == (run_bin ? 8'h29 : 8'h5A))
      dismissed <= 1;
  end

  wire        want = loaded && !playing && (load_a || load_b || (run && !dismissed && !recorded));
  wire        t_bin  = load_b ? b_bin : a_bin;          // file being loaded
  wire [47:0] t_name = load_b ? b_name : a_name;
  wire        prev_bin = load_b ? a_bin : 1'b0;        // BASIC after power-on
  wire [15:0] r_addr = b_valid ? b_addr : a_addr;
  wire        ld = load_a || load_b;

  // Name length without trailing spaces.
  function automatic logic [2:0] name_len(input logic [47:0] n);
    name_len = 0;
    for (int i = 0; i < 6; i++) if (n[47 - 8 * i -: 8] != 8'h20) name_len = 3'(i + 1);
  endfunction
  function automatic logic [7:0] hex(input logic [3:0] n);
    return n < 4'd10 ? 8'h30 + {4'd0, n} : 8'h37 + {4'd0, n};
  endfunction
  wire [2:0] nlen = name_len(t_name);

  // Every possible piece in fixed slots, with an include flag each; a
  // sequencer copies the included ones into `text`, one slot per clock.
  logic [5:0]   sc_idx;
  logic [4:0]   w;
  logic [175:0] build;
  logic [7:0]   sc;
  logic         si;
  always_comb begin
    sc = 8'h20; si = 0;
    case (sc_idx)
      0: begin sc = "T"; si = 1; end  1: begin sc = "Y"; si = 1; end
      2: begin sc = "P"; si = 1; end  3: begin sc = "E"; si = 1; end
      4: begin sc = " "; si = 1; end
      5: begin sc = "M"; si = ld && t_bin && !prev_bin; end
      6: begin sc = "O"; si = ld && t_bin && !prev_bin; end
      7: begin sc = "N"; si = ld && t_bin && !prev_bin; end
      8: begin sc = " "; si = ld && t_bin && !prev_bin; end
      9: begin sc = `TH_RETURN; si = ld && t_bin && !prev_bin; end
      10: begin sc = " "; si = ld && t_bin && !prev_bin; end
      11: begin sc = "E"; si = ld && !t_bin && prev_bin; end
      12: begin sc = " "; si = ld && !t_bin && prev_bin; end
      13: begin sc = `TH_RETURN; si = ld && !t_bin && prev_bin; end
      14: begin sc = " "; si = ld && !t_bin && prev_bin; end
      15: begin sc = "L"; si = ld && t_bin; end
      16: begin sc = " "; si = ld && t_bin; end
      17: begin sc = "L"; si = ld && !t_bin; end
      18: begin sc = "O"; si = ld && !t_bin; end
      19: begin sc = "A"; si = ld && !t_bin; end
      20: begin sc = "D"; si = ld && !t_bin; end
      21: begin sc = " "; si = ld && !t_bin; end
      22, 23, 24, 25, 26, 27: begin
        sc = t_name[47 - 8 * (sc_idx - 22) -: 8];
        si = ld && (sc_idx - 22) < nlen;
      end
      28: begin sc = " "; si = ld; end
      29: begin sc = t_bin ? `TH_SPACE : `TH_RETURN; si = ld; end
      30: begin sc = "G"; si = !ld && run_bin; end
      31: begin sc = " "; si = !ld && run_bin; end
      32: begin sc = hex(r_addr[15:12]); si = !ld && run_bin; end
      33: begin sc = hex(r_addr[11:8]);  si = !ld && run_bin; end
      34: begin sc = hex(r_addr[7:4]);   si = !ld && run_bin; end
      35: begin sc = hex(r_addr[3:0]);   si = !ld && run_bin; end
      36: begin sc = "R"; si = !ld && !run_bin; end
      37: begin sc = "U"; si = !ld && !run_bin; end
      38: begin sc = "N"; si = !ld && !run_bin; end
      39: begin sc = " "; si = !ld; end
      40: begin sc = run_bin ? `TH_SPACE : `TH_RETURN; si = !ld; end
      default: ;
    endcase
  end

  initial begin sc_idx = 0; w = 0; build = 0; show = 0; len = 0; text = 0; end

  always_ff @(posedge clk) begin
    if (sc_idx == 6'(SLOTS - 1)) begin
      text <= build;
      len  <= w;
      show <= want;
      sc_idx <= 0;
      w <= 0;
      build <= {22{8'h20}};
    end else begin
      if (si && w < 5'd22) begin
        build[175 - 8 * w -: 8] <= sc;
        w <= w + 5'd1;
      end
      sc_idx <= sc_idx + 6'd1;
    end
  end
endmodule

module tape_status (
  input  logic         clk,
  input  logic         loaded,      // a tape is mounted
  input  logic         playing,     // deck running (or briefly stopped mid-load)
  input  logic         fast,        // turbo active
  input  logic  [24:0] position,    // bytes played (0 after mount/rewind)
  input  logic  [24:0] length,      // bytes on the tape
  input  logic         hint_show,   // from tape_hint
  input  logic   [4:0] hint_len,
  input  logic [175:0] hint_text,
  // to the video clock
  output logic [223:0] status,      // held while req != ack
  output logic         req,
  input  logic         ack          // asynchronous; synchronised here
);
  localparam logic [8:0] BITS_PER_BYTE = 9'd11;
  localparam logic [8:0] BAUD = 9'd300;

  logic [15:0] elapsed, total;     // BCD m1 m0 s1 s0
  logic [8:0]  frac;               // bit-times into the current second
  logic [24:0] last_pos, last_len, cell_count;
  logic [29:0] rem;                // bit-times left to count into `total`
  logic [6:0]  fill;
  logic        computing;
  logic [1:0]  ack_sync;

  function automatic logic [15:0] bcd_tick(input logic [15:0] t);
    logic [15:0] n;
    n = t;
    if (t == 16'h9959) return t;                 // saturate at 99:59
    if (t[3:0] != 4'd9) n[3:0] = t[3:0] + 4'd1;
    else begin
      n[3:0] = 0;
      if (t[7:4] != 4'd5) n[7:4] = t[7:4] + 4'd1;
      else begin
        n[7:4] = 0;
        if (t[11:8] != 4'd9) n[11:8] = t[11:8] + 4'd1;
        else begin n[11:8] = 0; n[15:12] = t[15:12] + 4'd1; end
      end
    end
    return n;
  endfunction

  wire [24:0] cell_bytes = length[24:6] == 0 ? 25'd1 : {6'd0, length[24:6]};
  wire [223:0] pack = {hint_show, hint_len, hint_text, loaded, playing, fast, fill, elapsed, total};

  initial begin
    elapsed = 0; total = 0; frac = 0; last_pos = 0; last_len = 0;
    rem = 0; cell_count = 0; fill = 0; computing = 0; req = 0; status = 0; ack_sync = 0;
  end

  always_ff @(posedge clk) begin
    ack_sync <= {ack_sync[0], ack};
    last_pos <= position;
    // Elapsed time and fill follow the byte pointer; it only steps by one or
    // returns to zero (mount, rewind).
    if (position == 0 || position < last_pos) begin
      elapsed <= 0; frac <= 0; fill <= 0; cell_count <= 0;
    end else if (position != last_pos) begin
      if (frac + BITS_PER_BYTE >= BAUD) begin
        frac <= frac + BITS_PER_BYTE - BAUD;
        elapsed <= bcd_tick(elapsed);
      end else frac <= frac + BITS_PER_BYTE;
      if (cell_count + 25'd1 >= cell_bytes) begin
        cell_count <= 0;
        if (fill != 7'd64) fill <= fill + 7'd1;
      end else cell_count <= cell_count + 25'd1;
    end
    // Total time: subtract one second of bit-times per clock after a new tape.
    last_len <= length;
    if (length != last_len) begin
      rem <= length * BITS_PER_BYTE; total <= 0; computing <= 1;
    end else if (computing) begin
      if (rem >= {21'd0, BAUD}) begin rem <= rem - {21'd0, BAUD}; total <= bcd_tick(total); end
      else computing <= 0;
    end
    // Send a changed status once the previous one was taken.
    if (req == ack_sync[1] && pack != status) begin
      status <= pack;
      req <= !req;
    end
  end
endmodule

module tape_overlay #(
  parameter logic [7:0] FACTOR_BCD = 8'h32   // shown as "x32" while turbo is on
)(
  input  logic         clk,         // video clock
  input  logic         ce_pix,
  input  logic         hblank, vblank,
  input  logic         enable,      // OSD Tape overlay: On
  input  logic [223:0] status,      // from tape_status, stable while req != ack
  input  logic         req,
  output logic         ack,
  input  logic  [23:0] rgb_in,
  output logic  [23:0] rgb_out
);
  localparam int CHARS = 22;
  logic [1:0]   req_sync;
  logic [223:0] shown;
  logic [10:0]  x, width;
  logic [9:0]   y, height;
  logic         line_active, old_hblank, old_vblank;

  initial begin
    req_sync = 0; ack = 0; shown = 0; x = 0; y = 0; width = 0; height = 0;
    line_active = 0; old_hblank = 0; old_vblank = 0;
  end

  // Measure the active picture each frame (320x240 with the MP-1710 border,
  // 256x192 in the borderless sim build).
  wire line_end = ce_pix && hblank && !old_hblank;
  always_ff @(posedge clk) begin
    req_sync <= {req_sync[0], req};
    if (req_sync[1] != ack) begin shown <= status; ack <= req_sync[1]; end
    if (ce_pix) begin
      old_hblank <= hblank; old_vblank <= vblank;
      if (!hblank && !vblank) begin x <= x + 11'd1; line_active <= 1; end
      if (hblank && !old_hblank) begin
        if (line_active) begin width <= x; y <= y + 10'd1; end
        x <= 0; line_active <= 0;
      end
      if (vblank && !old_vblank) begin height <= y; y <= 0; end
    end
  end

  wire         hint_show = shown[223];
  wire [4:0]   hint_len = shown[222:218];
  wire [175:0] hint_text = shown[217:42];
  wire         tape_bar = enable && shown[41] && shown[40];   // loaded, playing
  wire         hint = enable && hint_show && !tape_bar;
  wire         visible = tape_bar || hint;
  wire         fast = shown[39];
  wire [6:0]   fill = shown[38:32];
  wire [15:0]  elapsed = shown[31:16], total = shown[15:0];
  // Double the overlay on wide or tall pictures (never on this core's 320x240).
  wire         sx = width >= 11'd512, sy = height >= 10'd300;
  wire [4:0]   chars = tape_bar ? 5'(CHARS) : hint_len;
  wire [10:0]  box_w = {3'd0, chars, 3'd0} + 11'd4;
  wire [9:0]   box_h = tape_bar ? 10'd18 : 10'd12;
  wire [10:0]  x0 = 11'd4;
  wire [9:0]   y0 = height - (sy ? {box_h[8:0], 1'b0} : box_h) - 10'd4;
  wire [10:0]  dx = x - x0;
  wire [9:0]   dy = y - y0;
  wire [10:0]  lx = sx ? {1'b0, dx[10:1]} : dx;
  wire [9:0]   ly = sy ? {1'b0, dy[9:1]} : dy;
  wire         in_box = x >= x0 && y >= y0 && lx < box_w && ly < box_h;

  // Text row at ly 2..9, x 2..: one character per 8 pixels.
  wire [10:0] tx = lx - 11'd2;
  wire [9:0]  ty = ly - 10'd2;

  function automatic logic [7:0] digit(input logic [3:0] d, input logic blank_zero);
    return blank_zero && d == 0 ? 8'h20 : 8'h30 + {4'd0, d};
  endfunction
  function automatic logic [7:0] char_at(input logic [4:0] col);
    if (!tape_bar) return hint_text[175 - 8 * col -: 8];
    case (col)
      5'd0: return "T";  5'd1: return "A";  5'd2: return "P";  5'd3: return "E";
      5'd5: return `TH_PLAY;
      5'd7: return digit(elapsed[15:12], 1'b1);  5'd8: return digit(elapsed[11:8], 1'b0);
      5'd9: return ":";
      5'd10: return digit(elapsed[7:4], 1'b0);  5'd11: return digit(elapsed[3:0], 1'b0);
      5'd12: return "/";
      5'd13: return digit(total[15:12], 1'b1);  5'd14: return digit(total[11:8], 1'b0);
      5'd15: return ":";
      5'd16: return digit(total[7:4], 1'b0);  5'd17: return digit(total[3:0], 1'b0);
      5'd19: return fast ? "x" : " ";
      5'd20: return fast ? digit(FACTOR_BCD[7:4], 1'b1) : " ";
      5'd21: return fast ? digit(FACTOR_BCD[3:0], 1'b0) : " ";
      default: return " ";
    endcase
  endfunction
  function automatic logic [7:0] special(input logic [7:0] c, input logic [2:0] r);
    logic [63:0] g;
    case (c)
      `TH_RETURN: g = 64'h0002022262FE6020;
      `TH_SPACE:  g = 64'h00000000008282FE;
      default:    g = 64'h4060707870604000;   // play
    endcase
    return g[63 - 8 * r -: 8];
  endfunction

  // Glyph rows for the coming line, fetched from the character ROM during
  // horizontal blanking (y has already stepped to the new line by then).
  logic [7:0] row [0:CHARS-1];
  logic [4:0] fi, fi_d;
  logic       fetching, fetch_d, start;
  logic [7:0] fc, fc_d;
  wire  [7:0] font_q;
  initial begin fi = 0; fi_d = 0; fetching = 0; fetch_d = 0; start = 0; fc_d = 0; end

  always_comb fc = char_at(fi);

  rom #(.MEMFILE("rom0.mem"), .DW(8), .AW(11)) font(
    .clk(clk),
    .addr({fc, ty[2:0]}),
    .dout(font_q),
    .ce(1'b1)
  );

  always_ff @(posedge clk) begin
    start <= line_end;
    if (start) begin fetching <= 1; fi <= 0; end
    else if (fetching) begin
      if (fi == 5'(CHARS - 1)) fetching <= 0;
      fi <= fi + 5'd1;
    end
    fetch_d <= fetching;
    fi_d <= fi;
    fc_d <= fc;
    if (fetch_d)
      row[fi_d] <= fc_d >= `TH_RETURN && fc_d <= `TH_PLAY ? special(fc_d, ty[2:0]) : font_q;
  end

  wire [4:0] col = tx[7:3];
  wire [7:0] glyph_row = row[col];
  wire text_on = ty < 10'd8 && tx < {3'd0, chars, 3'd0} && glyph_row[3'd7 - tx[2:0]];
  // Progress bar at ly 12..15, 128 px for 64 steps.
  wire bar_area = tape_bar && ly >= 10'd12 && ly < 10'd16 && lx >= 11'd2 && lx < 11'd130;
  wire bar_on = bar_area && (lx - 11'd2) < {3'd0, fill, 1'b0};

  always_comb begin
    rgb_out = rgb_in;
    if (visible && in_box)
      rgb_out = text_on ? 24'hFFFFFF : bar_on ? 24'h44CC44 : bar_area ? 24'h333333 : 24'h000000;
  end
endmodule
