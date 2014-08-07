module mux2 (
	input [1:0]in,
	output reg out,
	input [0:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		1'd0: out = in[0];
		1'd1: out = in[1];
	endcase
end

endmodule

module mux3 (
	input [2:0]in,
	output reg out,
	input [1:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		2'd0: out = in[0];
		2'd1: out = in[1];
		2'd2: out = in[2];
	endcase
end

endmodule

module mux4 (
	input [3:0]in,
	output reg out,
	input [1:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		2'd0: out = in[0];
		2'd1: out = in[1];
		2'd2: out = in[2];
		2'd3: out = in[3];
	endcase
end

endmodule

module mux5 (
	input [4:0]in,
	output reg out,
	input [2:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		3'd0: out = in[0];
		3'd1: out = in[1];
		3'd2: out = in[2];
		3'd3: out = in[3];
		3'd4: out = in[4];
	endcase
end

endmodule

module mux6 (
	input [5:0]in,
	output reg out,
	input [2:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		3'd0: out = in[0];
		3'd1: out = in[1];
		3'd2: out = in[2];
		3'd3: out = in[3];
		3'd4: out = in[4];
		3'd5: out = in[5];
	endcase
end

endmodule

module mux7 (
	input [6:0]in,
	output reg out,
	input [2:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		3'd0: out = in[0];
		3'd1: out = in[1];
		3'd2: out = in[2];
		3'd3: out = in[3];
		3'd4: out = in[4];
		3'd5: out = in[5];
		3'd6: out = in[6];
	endcase
end

endmodule

module mux8 (
	input [7:0]in,
	output reg out,
	input [2:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		3'd0: out = in[0];
		3'd1: out = in[1];
		3'd2: out = in[2];
		3'd3: out = in[3];
		3'd4: out = in[4];
		3'd5: out = in[5];
		3'd6: out = in[6];
		3'd7: out = in[7];
	endcase
end

endmodule

module mux9 (
	input [8:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
	endcase
end

endmodule

module mux10 (
	input [9:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
		4'd9: out = in[9];
	endcase
end

endmodule

module mux11 (
	input [10:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
		4'd9: out = in[9];
		4'd10: out = in[10];
	endcase
end

endmodule

module mux12 (
	input [11:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
		4'd9: out = in[9];
		4'd10: out = in[10];
		4'd11: out = in[11];
	endcase
end

endmodule

module mux13 (
	input [12:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
		4'd9: out = in[9];
		4'd10: out = in[10];
		4'd11: out = in[11];
		4'd12: out = in[12];
	endcase
end

endmodule

module mux14 (
	input [13:0]in,
	output reg out,
	input [3:0]config_in
);

always @ (*) begin: MUX
	case(config_in)
		4'd0: out = in[0];
		4'd1: out = in[1];
		4'd2: out = in[2];
		4'd3: out = in[3];
		4'd4: out = in[4];
		4'd5: out = in[5];
		4'd6: out = in[6];
		4'd7: out = in[7];
		4'd8: out = in[8];
		4'd9: out = in[9];
		4'd10: out = in[10];
		4'd11: out = in[11];
		4'd12: out = in[12];
		4'd13: out = in[13];
	endcase
end

endmodule

