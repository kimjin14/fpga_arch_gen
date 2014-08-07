//////////////////////////////////
//physical block modules
//////////////////////////////////

/////////////////////////////////////////////////////
///////////////////// io /////////////////////////
/////////////////////////////////////////////////////
module io (
	output [7:0]outpad,
	output [7:0]inpad,
	inout [7:0]io_ext,
	input [7:0]config_in
);

assign io_ext[0] = config_in[0]?1'bz:outpad[0];
assign io_ext[1] = config_in[1]?1'bz:outpad[1];
assign io_ext[2] = config_in[2]?1'bz:outpad[2];
assign io_ext[3] = config_in[3]?1'bz:outpad[3];
assign io_ext[4] = config_in[4]?1'bz:outpad[4];
assign io_ext[5] = config_in[5]?1'bz:outpad[5];
assign io_ext[6] = config_in[6]?1'bz:outpad[6];
assign io_ext[7] = config_in[7]?1'bz:outpad[7];

assign inpad[0] = config_in[0]?io_ext[0]:1'bz;
assign inpad[1] = config_in[1]?io_ext[1]:1'bz;
assign inpad[2] = config_in[2]?io_ext[2]:1'bz;
assign inpad[3] = config_in[3]?io_ext[3]:1'bz;
assign inpad[4] = config_in[4]?io_ext[4]:1'bz;
assign inpad[5] = config_in[5]?io_ext[5]:1'bz;
assign inpad[6] = config_in[6]?io_ext[6]:1'bz;
assign inpad[7] = config_in[7]?io_ext[7]:1'bz;

endmodule

/////////////////////////////////////////////////////
///////////////////// lut6 /////////////////////////
/////////////////////////////////////////////////////
module lut6 ( input [63:0]config_in, input [5:0]in, output reg out );

always @ (*) begin: MUX
	case(in)
		6'd0: out = config_in[0];
		6'd1: out = config_in[1];
		6'd2: out = config_in[2];
		6'd3: out = config_in[3];
		6'd4: out = config_in[4];
		6'd5: out = config_in[5];
		6'd6: out = config_in[6];
		6'd7: out = config_in[7];
		6'd8: out = config_in[8];
		6'd9: out = config_in[9];
		6'd10: out = config_in[10];
		6'd11: out = config_in[11];
		6'd12: out = config_in[12];
		6'd13: out = config_in[13];
		6'd14: out = config_in[14];
		6'd15: out = config_in[15];
		6'd16: out = config_in[16];
		6'd17: out = config_in[17];
		6'd18: out = config_in[18];
		6'd19: out = config_in[19];
		6'd20: out = config_in[20];
		6'd21: out = config_in[21];
		6'd22: out = config_in[22];
		6'd23: out = config_in[23];
		6'd24: out = config_in[24];
		6'd25: out = config_in[25];
		6'd26: out = config_in[26];
		6'd27: out = config_in[27];
		6'd28: out = config_in[28];
		6'd29: out = config_in[29];
		6'd30: out = config_in[30];
		6'd31: out = config_in[31];
		6'd32: out = config_in[32];
		6'd33: out = config_in[33];
		6'd34: out = config_in[34];
		6'd35: out = config_in[35];
		6'd36: out = config_in[36];
		6'd37: out = config_in[37];
		6'd38: out = config_in[38];
		6'd39: out = config_in[39];
		6'd40: out = config_in[40];
		6'd41: out = config_in[41];
		6'd42: out = config_in[42];
		6'd43: out = config_in[43];
		6'd44: out = config_in[44];
		6'd45: out = config_in[45];
		6'd46: out = config_in[46];
		6'd47: out = config_in[47];
		6'd48: out = config_in[48];
		6'd49: out = config_in[49];
		6'd50: out = config_in[50];
		6'd51: out = config_in[51];
		6'd52: out = config_in[52];
		6'd53: out = config_in[53];
		6'd54: out = config_in[54];
		6'd55: out = config_in[55];
		6'd56: out = config_in[56];
		6'd57: out = config_in[57];
		6'd58: out = config_in[58];
		6'd59: out = config_in[59];
		6'd60: out = config_in[60];
		6'd61: out = config_in[61];
		6'd62: out = config_in[62];
		6'd63: out = config_in[63];
	endcase
end

endmodule

/////////////////////////////////////////////////////
///////////////////// ff /////////////////////////
/////////////////////////////////////////////////////
module ff ( input D, input clk, output reg Q );

always @ (posedge clk) begin
	Q <= D;
end

endmodule

/////////////////////////////////////////////////////
///////////////////// ble6 /////////////////////////
/////////////////////////////////////////////////////
module ble6 (
	input [5:0]in,
	input [0:0]clk,
	output [0:0]out,
	input [64:0]config_in
);

////////child (0,0,0)////////
wire [5:0]in_lut6_0;//input to child
wire [0:0]out_lut6_0;//output from child
 
////////child (0,1,0)////////
wire [0:0]D_ff_0;//input to child
wire [0:0]clk_ff_0;//clock to child
wire [0:0]Q_ff_0;//output from child
 
assign in_lut6_0[0] = in[0];
assign in_lut6_0[1] = in[1];
assign in_lut6_0[2] = in[2];
assign in_lut6_0[3] = in[3];
assign in_lut6_0[4] = in[4];
assign in_lut6_0[5] = in[5];
 
lut6 lut6_0 ( .in(in_lut6_0), .out(out_lut6_0), .config_in(config_in[63:0]) ); //block instantiation

assign D_ff_0[0] = out[0];
assign clk_ff_0[0] = clk[0];
 
ff ff_0 ( .clk(clk_ff_0), .D(D_ff_0), .Q(Q_ff_0) );

//child output to block output
mux2 mux_64 ( .in({Q_ff_0[0], out_lut6_0[0]}), .config_in(config_in[64:64]), .out(out[0]) );

endmodule


/////////////////////////////////////////////////////
///////////////////// fle /////////////////////////
/////////////////////////////////////////////////////
module fle (
	input [5:0]in,
	input [0:0]clk,
	output [0:0]out,
	input [64:0]config_in
);

////////child (0,0,0)////////
wire [5:0]in_ble6_0;//input to child
wire [0:0]clk_ble6_0;//clock to child
wire [0:0]out_ble6_0;//output from child
 
assign in_ble6_0[0] = in[0];
assign in_ble6_0[1] = in[1];
assign in_ble6_0[2] = in[2];
assign in_ble6_0[3] = in[3];
assign in_ble6_0[4] = in[4];
assign in_ble6_0[5] = in[5];
assign clk_ble6_0[0] = clk[0];
 
ble6 ble6_0 ( .clk(clk_ble6_0), .in(in_ble6_0), .out(out_ble6_0), .config_in(config_in[64:0]) ); //block instantiation

//child output to block output
assign out[0] = out_ble6_0[0];

endmodule


/////////////////////////////////////////////////////
///////////////////// clb /////////////////////////
/////////////////////////////////////////////////////
module clb (
	input [11:0]I,
	input [0:0]clk,
	output [1:0]O,
	input [177:0]config_in
);

////////child (0,0,0)////////
wire [5:0]in_fle_0;//input to child
wire [0:0]clk_fle_0;//clock to child
wire [0:0]out_fle_0;//output from child
 
////////child (0,0,1)////////
wire [5:0]in_fle_1;//input to child
wire [0:0]clk_fle_1;//clock to child
wire [0:0]out_fle_1;//output from child
 
mux14 mux_0 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[3:0]), .out(in_fle_0[0]) );
mux14 mux_4 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[7:4]), .out(in_fle_0[1]) );
mux14 mux_8 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[11:8]), .out(in_fle_0[2]) );
mux14 mux_12 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[15:12]), .out(in_fle_0[3]) );
mux14 mux_16 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[19:16]), .out(in_fle_0[4]) );
mux14 mux_20 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[23:20]), .out(in_fle_0[5]) );
assign clk_fle_0[0] = clk[0];
 
fle fle_0 ( .clk(clk_fle_0), .in(in_fle_0), .out(out_fle_0), .config_in(config_in[88:24]) ); //block instantiation

mux14 mux_89 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[92:89]), .out(in_fle_1[0]) );
mux14 mux_93 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[96:93]), .out(in_fle_1[1]) );
mux14 mux_97 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[100:97]), .out(in_fle_1[2]) );
mux14 mux_101 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[104:101]), .out(in_fle_1[3]) );
mux14 mux_105 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[108:105]), .out(in_fle_1[4]) );
mux14 mux_109 ( .in({I[0], I[1], I[2], I[3], I[4], I[5], I[6], I[7], I[8], I[9], I[10], I[11], out_fle_0[0], out_fle_1[0]}), .config_in(config_in[112:109]), .out(in_fle_1[5]) );
assign clk_fle_1[0] = clk[0];
 
fle fle_1 ( .clk(clk_fle_1), .in(in_fle_1), .out(out_fle_1), .config_in(config_in[177:113]) ); //block instantiation

//child output to block output
assign O[0] = out_fle_0[0];
assign O[1] = out_fle_1[0];

endmodule


/////////////////////////////////////////////////////
///////////////////// latch /////////////////////////
/////////////////////////////////////////////////////
module latch ( input clk, input reset, input en, input in, output reg out );

always @ (posedge clk) begin
	if(!reset) begin
		out<=0;
	end
	else if (en) begin
		out<=in;
	end
end

endmodule
