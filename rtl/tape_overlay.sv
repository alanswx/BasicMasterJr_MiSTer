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

module tape_status (
  input  logic        clk,
  input  logic        loaded,      // a tape is mounted
  input  logic        playing,     // deck running (or briefly stopped mid-load)
  input  logic        fast,        // turbo active
  input  logic [24:0] position,    // bytes played (0 after mount/rewind)
  input  logic [24:0] length,      // bytes on the tape
  // to the video clock
  output logic [41:0] status,      // held while req != ack
  output logic        req,
  input  logic        ack          // asynchronous; synchronised here
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
  wire [41:0] pack = {loaded, playing, fast, fill, elapsed, total};

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
  input  logic        clk,         // video clock
  input  logic        ce_pix,
  input  logic        hblank, vblank,
  input  logic        enable,      // OSD Tape overlay: On
  input  logic [41:0] status,      // from tape_status, stable while req != ack
  input  logic        req,
  output logic        ack,
  input  logic [23:0] rgb_in,
  output logic [23:0] rgb_out
);
  localparam int CHARS = 22;
  logic [1:0]  req_sync;
  logic [41:0] shown;
  logic [10:0] x, width;
  logic [9:0]  y, height;
  logic        line_active, old_hblank, old_vblank;

  initial begin
    req_sync = 0; ack = 0; shown = 0; x = 0; y = 0; width = 0; height = 0;
    line_active = 0; old_hblank = 0; old_vblank = 0;
  end

  // Measure the active picture each frame (320x240 with the MP-1710 border,
  // 256x192 in the borderless sim build).
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

  wire        visible = enable && shown[41] && shown[40];
  wire        fast = shown[39];
  wire [6:0]  fill = shown[38:32];
  wire [15:0] elapsed = shown[31:16], total = shown[15:0];
  // Double the overlay on wide or tall pictures (never on this core's 320x240).
  wire        sx = width >= 11'd512, sy = height >= 10'd300;
  localparam int BOX_W = CHARS * 8 + 4, BOX_H = 18;
  wire [10:0] x0 = 11'd4;
  wire [9:0]  y0 = height - (sy ? 10'(2 * BOX_H) : 10'(BOX_H)) - 10'd4;
  wire [10:0] dx = x - x0;
  wire [9:0]  dy = y - y0;
  wire [10:0] lx = sx ? {1'b0, dx[10:1]} : dx;
  wire [9:0]  ly = sy ? {1'b0, dy[9:1]} : dy;
  wire        in_box = x >= x0 && y >= y0 && lx < 11'(BOX_W) && ly < 10'(BOX_H);

  // Text row at ly 2..9, x 2..; codes 0-9 digits, 10 ':', 11 '/', 12 T,
  // 13 A, 14 P, 15 E, 16 x, 17 play, 18 space.
  wire [10:0] tx = lx - 11'd2;
  wire [9:0]  ty = ly - 10'd2;
  wire [4:0]  col = tx[7:3];
  function automatic logic [4:0] digit(input logic [3:0] d, input logic blank_zero);
    return blank_zero && d == 0 ? 5'd18 : {1'b0, d};
  endfunction
  logic [4:0] code;
  always_comb begin
    case (col)
      5'd0: code = 5'd12;  5'd1: code = 5'd13;  5'd2: code = 5'd14;  5'd3: code = 5'd15;
      5'd4: code = 5'd18;  5'd5: code = 5'd17;  5'd6: code = 5'd18;
      5'd7: code = digit(elapsed[15:12], 1'b1);  5'd8: code = {1'b0, elapsed[11:8]};
      5'd9: code = 5'd10;  5'd10: code = {1'b0, elapsed[7:4]};  5'd11: code = {1'b0, elapsed[3:0]};
      5'd12: code = 5'd11;
      5'd13: code = digit(total[15:12], 1'b1);  5'd14: code = {1'b0, total[11:8]};
      5'd15: code = 5'd10;  5'd16: code = {1'b0, total[7:4]};  5'd17: code = {1'b0, total[3:0]};
      5'd18: code = 5'd18;
      5'd19: code = fast ? 5'd16 : 5'd18;
      5'd20: code = fast ? digit(FACTOR_BCD[7:4], 1'b1) : 5'd18;
      5'd21: code = fast ? {1'b0, FACTOR_BCD[3:0]} : 5'd18;
      default: code = 5'd18;
    endcase
  end
  function automatic logic [7:0] glyph(input logic [4:0] c, input logic [2:0] r);
    logic [63:0] g;
    case (c)
      5'd0:  g = 64'h3C666E7666663C00;  5'd1:  g = 64'h1838181818187E00;
      5'd2:  g = 64'h3C66060C30607E00;  5'd3:  g = 64'h3C66061C06663C00;
      5'd4:  g = 64'h0C1C3C6C7E0C0C00;  5'd5:  g = 64'h7E607C0606663C00;
      5'd6:  g = 64'h1C30607C66663C00;  5'd7:  g = 64'h7E060C1830303000;
      5'd8:  g = 64'h3C66663C66663C00;  5'd9:  g = 64'h3C66663E060C3800;
      5'd10: g = 64'h0018180018180000;  5'd11: g = 64'h02060C1830604000;
      5'd12: g = 64'h7E18181818181800;  5'd13: g = 64'h183C66667E666600;
      5'd14: g = 64'h7C66667C60606000;  5'd15: g = 64'h7E60607C60607E00;
      5'd16: g = 64'h0000663C183C6600;  5'd17: g = 64'h4060707870604000;
      default: g = 64'h0;
    endcase
    return g[63 - 8 * r -: 8];
  endfunction
  wire [7:0] glyph_row = glyph(code, ty[2:0]);
  wire text_on = ty < 10'd8 && tx < 11'(CHARS * 8) && glyph_row[3'd7 - tx[2:0]];
  // Progress bar at ly 12..15, 128 px for 64 steps.
  wire bar_area = ly >= 10'd12 && ly < 10'd16 && lx >= 11'd2 && lx < 11'd130;
  wire bar_on = bar_area && (lx - 11'd2) < {3'd0, fill, 1'b0};

  always_comb begin
    rgb_out = rgb_in;
    if (visible && in_box)
      rgb_out = text_on ? 24'hFFFFFF : bar_on ? 24'h44CC44 : bar_area ? 24'h333333 : 24'h000000;
  end
endmodule
