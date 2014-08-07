module add16 (a,b,z);
	input [15:0] a;
	input [15:0] b;
	output [16:0] z;

	assign z = a+b;
	
endmodule
