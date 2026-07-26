
module video(
  input clk,

  output reg hs,
  output reg vs,
  output reg hb,
  output reg vb,

  output reg [8:0] hcount,
  output reg [8:0] vcount,

  output reg [7:0] red,
  output reg [7:0] green,
  output reg [7:0] blue,

  output reg [15:0] ram_addr,
  output reg [10:0] char_addr,
  input [7:0] ram_data,
  input [7:0] char_data,

  output reg frame,
  output reg ce_pix,

  input color_board,
  input inverted,

  // $EFE0. bit6 = graphics enabled, bit7 = graphics ONLY (clear = mixed with
  // text), bits[3:0] = page. Both $40-$4F (mixed) and $C0-$CF (graphics only)
  // have bit 6 set -- that is the enable; bit 7 picks between them. MAME only
  // ever tests bit 7 and calls bit 6 "unknown, set in tandem".
  //   base = $0900 + $200 * (mode & $0F); page 1 $0900, page 2 $2100.
  // Bitmap bytes are row-interleaved per character cell -- the 8 scanlines of
  // one cell are 32 bytes apart. [bm2 depend.c:259-285, 327-370]
  input [7:0] screen_mode,

  input [7:0] charcolor,
  input [9:0] color_ram_addr,
  input color_ram_wr

);

// 256x192

initial begin
  hs = 1'b1;
  vs = 1'b1;
end

reg [7:0] color_ram[32*24:0];

// ---------------------------------------------------------------------------
// Character-cell renderer
//
// Runs free of the display scan, filling `vram` one pixel at a time. px/py walk
// the 256x192 picture; each pixel needs the cell's attribute, its pattern byte
// (character generator in text mode, RAM in graphics mode) and one bit of it.
// ---------------------------------------------------------------------------
reg [1:0] state;
reg [7:0] px, py;
reg [2:0] vram[256*192:0];

wire [9:0] cell_idx = { py[7:3], px[7:3] };          // (py/8)*32 + (px/8)
wire [15:0] gfx_base = 16'h0900 + { screen_mode[3:0], 9'd0 };  // $900 + $200*n
wire graphics_en   = screen_mode[6];
wire graphics_only = screen_mode[7];

// In mixed mode bm2 alternates whole frames between the text page and the
// bitmap; whether real hardware overlays them instead is unconfirmed.
reg mix_phase;
wire show_gfx = graphics_en & (graphics_only | mix_phase);

reg [7:0] color;
reg [7:0] pattern;

// Screen reverse rotates the whole attribute byte by a nibble, so it swaps the
// two blink flags along with the two colours. [bm2 depend.c:240]
wire [7:0] attr = inverted ? { color[3:0], color[7:4] } : color;

// Blink: bit 3 blanks the foreground on one phase, bit 7 blanks the background
// on the other. [bm2 depend.c:150-160]
reg blink;
wire [7:0] attr_b = blink ? (attr[3] ? (attr & 8'h70) : (attr & 8'h77))
                          : (attr[7] ? (attr & 8'h07) : (attr & 8'h77));
wire [2:0] fg = attr_b[2:0];
wire [2:0] bg = attr_b[6:4];

always @(posedge clk) begin
  color <= color_ram[cell_idx];
  if (color_ram_wr) color_ram[color_ram_addr] <= charcolor;
end

always @(posedge clk) begin
  case (state)
    2'd0: begin
      // Graphics mode reads the bitmap directly; text mode reads the character
      // code and looks it up in the generator on the next state.
      ram_addr <= show_gfx ? (gfx_base + { py[7:3], 8'd0 } + { py[2:0], 5'd0 } + { 8'd0, px[7:3] })
                           : (16'h100 + { 6'd0, cell_idx });
      state <= 2'd1;
    end
    2'd1: begin
      char_addr <= { ram_data, py[2:0] };
      pattern <= ram_data;
      state <= 2'd2;
    end
    2'd2: begin
      begin
        reg bit_on;
        bit_on = show_gfx ? pattern[3'd7-px[2:0]] : char_data[3'd7-px[2:0]];

        if (color_board)
          vram[py*256+px] <= bit_on ? fg : bg;
        else
          vram[py*256+px] <= bit_on ?
            inverted ? 3'b000 : 3'b111 :
            inverted ? 3'b111 : 3'b000;
      end

      px <= px + 8'd1;
      if (px == 255) begin
        py <= py + 8'd1;
        if (py == 8'd191) py <= 8'd0;
      end
      state <= 2'd0;
    end
  endcase
end

// ---------------------------------------------------------------------------
// Display scan
// ---------------------------------------------------------------------------
reg [15:0] vram_addr;
reg [2:0] vram_data;
reg [5:0] blink_div;

always @(posedge clk) begin

  frame <= 1'b0;
  ce_pix <= ~ce_pix;

  if (~ce_pix) begin

    vram_addr <= vcount * 256 + hcount;
    vram_data <= vram[vram_addr];

  end

  else begin

    if (~(hb|vb)) begin
      red <= vram_data[2] ? 8'hff : 8'd0;
      green <= vram_data[1] ? 8'hff : 8'd0;
      blue <= vram_data[0] ? 8'hff : 8'd0;
    end

    hcount <= hcount + 9'd1;
    case (hcount)
      255: hb <= 1'b1;
      271: hs <= 1'b0;
      311: hs <= 1'b1;
      343: begin
        vcount <= vcount + 9'd1;
        hcount <= 9'b0;
        hb <= 1'b0;
        case (vcount)
          191: vb <= 1'b1;
          215: vs <= 1'b0;
          249: vs <= 1'b1;
          261: begin
            vcount <= 9'd0;
            vb <= 1'b0;
            frame <= 1'b1;
            mix_phase <= ~mix_phase;
            // ~1 Hz attribute blink: flip every 30 frames.
            blink_div <= blink_div + 6'd1;
            if (blink_div == 6'd29) begin
              blink_div <= 6'd0;
              blink <= ~blink;
            end
          end
        endcase
      end
    endcase

  end

end

endmodule
