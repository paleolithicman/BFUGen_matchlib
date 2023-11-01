`timescale 1ns/1ns

module tb;

logic clk;
logic rst;

reg [31:0] bt0;
reg [31:0] bt1;
wire stream_out_t_val;
reg stream_out_t_rdy;
wire [522:0] stream_out_t_msg;
reg cmd_in_t_val;
wire cmd_in_t_rdy;
reg [35:0] cmd_in_t_msg;
wire bfu_out_t_val;
reg bfu_out_t_rdy;
wire [36:0] bfu_out_t_msg;
wire bfu_rdreq_t_val;
reg bfu_rdreq_t_rdy;
wire [13:0] bfu_rdreq_t_msg;
reg bfu_rdrsp_t_val;
wire bfu_rdrsp_t_rdy;
reg [383:0] bfu_rdrsp_t_msg;

logic [191:0] regfile [0:15];

initial begin
    clk = 1'b0;
    forever #2 clk = ~clk;
end

initial begin
    rst = 'b1;
    stream_out_t_rdy = 'b1;
    bfu_out_t_rdy = 'b1;
    cmd_in_t_val = 'b0;
    cmd_in_t_msg = 'b0;
    bt0 = 0;
    bt1 = 32;
    regfile[0] = 192'h000000000000000000000000000000000000000000000000;
    regfile[1] = 192'h0000000000000000000088f700000000beef00000000dead;
    regfile[2] = 192'h0000000000000000000000000000cafe0000010000081000;
    regfile[3] = 192'h0000000000000000aaaa0000beef00000000000000000000;
    regfile[4] = 192'h000000000000000000040003000200010004000300020001;
    regfile[5] = 192'h000000000000000000040003000200010004000300020001;
    regfile[6] = 192'h000000000000000000040003000200010004000300020001;
    regfile[7] = 192'h000000000000000000040003000200010004000300020001;
    regfile[8] = 192'h000000000000000000040003000200010004000300020001;
    regfile[9] = 192'h000000000000000000040003000200010004000300020001;
    regfile[10] = 192'h000000000000000000040003000200010004000300020001;
    regfile[11] = 192'h000000000000000000040003000200010004000300020001;
    regfile[12] = 192'h000000000000000000040003000200010004000300020001;
    #40;
    rst = 'b0;
    #40;
    cmd_in_t_val = 'b1;
    cmd_in_t_msg = 36'h000000090;
    #4;
    while (!cmd_in_t_rdy) begin
        #4;
    end
    cmd_in_t_val = 'b0;
    while (!bfu_out_t_val) begin
        #4;
    end
    cmd_in_t_val = 'b1;
    cmd_in_t_msg = 36'h000000091;
    #4;
    while (!cmd_in_t_rdy) begin
        #4;
    end
    cmd_in_t_val = 'b0;

    #100;
    $finish;
end

always_comb begin
    bfu_rdreq_t_rdy = bfu_rdrsp_t_rdy;
end

always_ff @(posedge clk) begin
    if (rst) begin
        bfu_rdrsp_t_val <= 1'b0;
        bfu_rdrsp_t_msg <= 'b0;
    end else begin
        if (bfu_rdrsp_t_rdy) begin
            bfu_rdrsp_t_val <= 'b0;
            if (bfu_rdreq_t_val) begin
                bfu_rdrsp_t_msg[191:0] <= regfile[bfu_rdreq_t_msg[8:4]];
                bfu_rdrsp_t_msg[383:192] <= regfile[bfu_rdreq_t_msg[13:9]];
                bfu_rdrsp_t_val <= 'b1;
            end
        end
    end
end

outputUnit outputUnit_inst(
  .i_clk(clk), 
  .i_rst(rst), 
  .bt0(bt0), 
  .bt1(bt1), 
  .stream_out_t_val(stream_out_t_val), 
  .stream_out_t_rdy(stream_out_t_rdy), 
  .stream_out_t_msg(stream_out_t_msg), 
  .cmd_in_t_val(cmd_in_t_val),
  .cmd_in_t_rdy(cmd_in_t_rdy), 
  .cmd_in_t_msg(cmd_in_t_msg), 
  .bfu_out_t_val(bfu_out_t_val), 
  .bfu_out_t_rdy(bfu_out_t_rdy), 
  .bfu_out_t_msg(bfu_out_t_msg), 
  .bfu_rdreq_t_val(bfu_rdreq_t_val),
  .bfu_rdreq_t_rdy(bfu_rdreq_t_rdy), 
  .bfu_rdreq_t_msg(bfu_rdreq_t_msg), 
  .bfu_rdrsp_t_val(bfu_rdrsp_t_val), 
  .bfu_rdrsp_t_rdy(bfu_rdrsp_t_rdy), 
  .bfu_rdrsp_t_msg(bfu_rdrsp_t_msg)
);

endmodule
