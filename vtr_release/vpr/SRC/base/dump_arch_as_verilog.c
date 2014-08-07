#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "pb_type_graph.h"
#include "rr_graph.h"
#include "rr_graph_util.h"
#include "dump_arch_as_verilog.h"
#include "dump_bitstream.h"
#include "vpr_utils.h"

void dump_arch_as_verilog () {
	
	int *pb_graph_config_bits = (int*)malloc(num_types*sizeof(int));
	int *mux_max_size = (int*)calloc(1, sizeof(int));
	dump_pb_graph_as_verilog("fpga_modules.v", pb_graph_config_bits, mux_max_size);
	dump_rr_graph_as_verilog("fpga_top.v", pb_graph_config_bits, mux_max_size);
	(*mux_max_size)++; // for GROUND signal for the mux
	dump_mux_as_verilog("fpga_mux.v", mux_max_size);

}

void dump_mux_as_verilog (INP const char *file_name, int* mux_max_size) {
	FILE *fp;
	fp = my_fopen(file_name, "w", 0);
	/////////////////////////////////////////////////////////////////////	
	//create muxes
	/////////////////////////////////////////////////////////////////////
	for (int i=2; i<mux_max_size[0]+1; i++) {
		fprintf(fp, "module mux%d (\n", i);
		fprintf(fp, "\tinput [%d:0]in,\n", i-1);
		fprintf(fp, "\toutput reg out,\n");		
		fprintf(fp, "\tinput [%d:0]config_in\n", (int)ceil(log(i)/log(2))-1);
		fprintf(fp, ");\n\n");
		fprintf(fp, "always @ (*) begin: MUX\n");		
		fprintf(fp, "\tcase(config_in)\n");	
		for (int j=0; j<i; j++) {
			fprintf(fp, "\t\t%d'd%d: out = in[%d];\n", (int)ceil(log(i)/log(2)), j, j);
		}		
		fprintf(fp, "\tendcase\n");	
		fprintf(fp, "end\n\n");				
		fprintf(fp, "endmodule\n\n");
	}
	fclose(fp);
}
	
/* This function calls the recursive dump pb function for all the types.
file_name: where the verilog for the pb will be dumped
pb_graph_config_bits: used to keep information on how many config bits are required */

void dump_pb_graph_as_verilog (INP const char *file_name, int* pb_graph_config_bits,\
	int* mux_max_size) {
	
	FILE *fp;
	int i;

	fp = my_fopen(file_name, "w", 0);
	
	fprintf(fp, "//////////////////////////////////\n");
	fprintf(fp, "//physical block modules\n");
	fprintf(fp, "//////////////////////////////////\n\n");
	
	//for all blocks (not <EMPTY>), recursively dump physcal blocks
	for (i = 0; i < num_types; i++) {
		if (type_descriptors[i].pb_graph_head) {
			pb_graph_config_bits[i] = dump_pb_rec_verilog(type_descriptors[i].pb_graph_head, \
				fp, type_descriptors[i].capacity, mux_max_size);						
		}
	}
	
	//print module latch
	fprintf(fp, "/////////////////////////////////////////////////////\n");
	fprintf(fp, "///////////////////// latch /////////////////////////\n");
	fprintf(fp, "/////////////////////////////////////////////////////\n");
	fprintf(fp, "module latch ( input clk, input reset, input en, input in, output reg out );\n\n");
	fprintf(fp, "always @ (posedge clk) begin\n");
	fprintf(fp, "\tif(!reset) begin\n\t\tout<=0;\n\tend\n");
	fprintf(fp, "\telse if (en) begin\n\t\tout<=in;\n\tend\n");
	fprintf(fp, "end\n\n");
	fprintf(fp, "endmodule\n");
	
	fclose(fp);
}

/* This function recursively prints out pb tree as modules (from the bottom of tree - lut and ff).
pb_graph_node: current node being printed
fp: file where the verilog is being dumped
capacity: capacity of the block (to group them together)

returns number of configuration bits required for node and its children */

int dump_pb_rec_verilog (const INP t_pb_graph_node *pb_graph_node, INP FILE *fp, \
	INP int capacity, int* mux_max_size) {

	int i, j, k, l;
	int n_config_prev_total, n_config;
	int *n_config_used; 
	int** n_config_prev_per_child;
	
	int flag_first;
	
	t_pb_graph_pin **pb_graph_pins;
	t_port *port;

	//////////////////////////////////////////////////////
	// Bottom of the tree (lut6 and ff)
	// - lut6 instead of lut to avoid route-through mode
	//////////////////////////////////////////////////////
	if (strcmp(pb_graph_node->pb_type->name, "lut6") == 0) {
		fprintf(fp, "/////////////////////////////////////////////////////\n");
		fprintf(fp, "///////////////////// lut6 /////////////////////////\n");
		fprintf(fp, "/////////////////////////////////////////////////////\n");
	
		fprintf(fp, "module lut6 ( input [63:0]config_in, input [5:0]in, output reg out );\n\n");	
		fprintf(fp, "wire [5:0]in_flip;\n");
		fprintf(fp, "assign in_flip = {in[0], in[1], in[2], in[3], in[4], in[5]};\n\n");
		fprintf(fp, "always @ (*) begin: MUX\n\tcase(in_flip)\n");			
		for (j=0; j<64; j++) {
			fprintf(fp, "\t\t6'd%d: out = config_in[%d];\n", j, j);
		}		
		fprintf(fp, "\tendcase\nend\n\n");					
		fprintf(fp, "endmodule\n\n");
		
		return 64; // 6-input LUT requires 64 configuration bits	
	} else if (strcmp(pb_graph_node->pb_type->name, "ff") == 0) {
		fprintf(fp, "/////////////////////////////////////////////////////\n");
		fprintf(fp, "///////////////////// ff /////////////////////////\n");
		fprintf(fp, "/////////////////////////////////////////////////////\n");
		fprintf(fp, "module ff ( input D, input clk, output reg Q );\n\n");
		fprintf(fp, "always @ (posedge clk) begin\n\tQ <= D;\nend\n\n");
		fprintf(fp, "endmodule\n\n");
		
		return 0; // ff requires 0 configuration bits
	} else if (strcmp(pb_graph_node->pb_type->name, "io") == 0) {
		fprintf(fp, "/////////////////////////////////////////////////////\n");
		fprintf(fp, "///////////////////// io /////////////////////////\n");
		fprintf(fp, "/////////////////////////////////////////////////////\n");
		fprintf(fp, "module io (\n");

		flag_first = 0;
		pb_graph_pins = pb_graph_node->input_pins;
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			if (flag_first == 0) flag_first = 1;
			else fprintf(fp, ",\n");
			port = pb_graph_pins[i][0].port;
			fprintf(fp, "\toutput [%d:0]%s", capacity*port->num_pins-1, port->name);
		}
		pb_graph_pins = pb_graph_node->output_pins;
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			if (flag_first == 0) flag_first = 1;
			else fprintf(fp, ",\n");
			port = pb_graph_pins[i][0].port;
			fprintf(fp, "\toutput [%d:0]%s", capacity*port->num_pins-1, port->name);
		}
		fprintf(fp, ",\n\tinout [%d:0]%s_ext", capacity-1, pb_graph_node->pb_type->name);	
		fprintf(fp, ",\n\tinput [%d:0]config_in", capacity-1);	
		fprintf(fp, "\n);\n\n");

		pb_graph_pins = pb_graph_node->input_pins;
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			port = pb_graph_pins[i][0].port;
			for (j=0; j<capacity; j++) {
				fprintf(fp, "assign %s_ext[%d] = config_in[%d]?1'bz:%s[%d];\n", \
					pb_graph_node->pb_type->name, j, j, port->name, j);	
			}
		}
		fprintf(fp, "\n");
		pb_graph_pins = pb_graph_node->output_pins;
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			port = pb_graph_pins[i][0].port;
			for (j=0; j<capacity; j++) {
				fprintf(fp, "assign %s[%d] = config_in[%d]?%s_ext[%d]:1'bz;\n", \
					port->name, j, j, pb_graph_node->pb_type->name, j);	
			}
		}
		fprintf(fp, "\n");
		fprintf(fp, "endmodule\n\n");
		return capacity;
	}
	
	//////////////////////////////////////////////////////
	// Recursively go down to bottom of list
	// - keep track of previous configuration bits
	//////////////////////////////////////////////////////
	n_config_prev_total = 0; 
	n_config_prev_per_child = (int**)malloc(pb_graph_node->pb_type->num_modes*sizeof(int*));
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		n_config_prev_per_child[i] = (int*)malloc(pb_graph_node->pb_type->modes[i].num_pb_type_children*sizeof(int));
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			n_config_prev_per_child[i][j] = pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb*\
				dump_pb_rec_verilog(&pb_graph_node->child_pb_graph_nodes[i][j][0], \
					fp, capacity, mux_max_size);
			n_config_prev_total+=n_config_prev_per_child[i][j];
		}	
	}
	
	//////////////////////////////////////////////////////
	// Figure out config bit count
	// - repeat but need this info before module declaration
	//////////////////////////////////////////////////////
	n_config = 0;
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				for (l = 0; l < pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports; l++) {
					port = pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port;		
					for (int h = 0; h < port->num_pins; h++) {
						if (pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][h].num_input_edges > 1) {
							n_config +=(int)ceil(((double)log(pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][h].num_input_edges)/log(2)));
						}
					}
				}
				for (l = 0; l < pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports; l++) {
					port = pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port;		
					for (int h = 0; h < port->num_pins; h++) {
						if (pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][h].num_input_edges > 1) {
							n_config +=(int)ceil(((double)log(pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][h].num_input_edges)/log(2)));
						}
					}
				}
			}
		}
	}
	for (l = 0; l < pb_graph_node->num_output_ports; l++) {
		port = pb_graph_node->output_pins[l][0].port;		
		for (int h = 0; h < port->num_pins; h++) {
			if (pb_graph_node->output_pins[l][h].num_input_edges > 1) {
				n_config +=(int)ceil(((double)log(pb_graph_node->output_pins[l][h].num_input_edges)/log(2)));
			}
		}
	}
	
	//////////////////////////////////////////////////////
	// Declare modules
	//////////////////////////////////////////////////////
	fprintf(fp, "/////////////////////////////////////////////////////\n");
	fprintf(fp, "///////////////////// %s /////////////////////////\n", pb_graph_node->pb_type->name);
	fprintf(fp, "/////////////////////////////////////////////////////\n");
	fprintf(fp, "module %s (\n", pb_graph_node->pb_type->name);
	
	//////////////////////////////////////////////////////
	// Declare input/output/clock pins
	//////////////////////////////////////////////////////
	flag_first = 0;

	pb_graph_pins = pb_graph_node->input_pins;
	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		if (flag_first == 0) flag_first = 1;
		else fprintf(fp, ",\n");
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\tinput [%d:0]%s", capacity*port->num_pins-1, port->name);
	}
	pb_graph_pins = pb_graph_node->clock_pins;
	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		if (flag_first == 0) flag_first = 1;
		else fprintf(fp, ",\n");
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\tinput [%d:0]%s", capacity*port->num_pins-1, port->name);
	}
	pb_graph_pins = pb_graph_node->output_pins;
	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		if (flag_first == 0) flag_first = 1;
		else fprintf(fp, ",\n");
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\toutput [%d:0]%s", capacity*port->num_pins-1, port->name);
	}
	//determine if configuration bits are needed
	//check children + itself has 0
	if (n_config+n_config_prev_total != 0) {
		if (flag_first == 0) flag_first = 1;
		else fprintf(fp, ",\n");
		fprintf(fp, "\tinput [%d:0]config_in", (n_config+n_config_prev_total)-1);	
	}
	fprintf(fp, "\n);\n\n");
	
	//////////////////////////////////////////////////////
	// Inside of the module
	//////////////////////////////////////////////////////
	flag_first = 0;
	n_config_used = (int*)calloc(1, sizeof(int));
	
	//go through all the immediate children
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				fprintf(fp, "////////child (%d,%d,%d)////////\n", i, j, k);
				
				//////////////////////////////////////////////////////
				// Connect module ports to instantiated blocks
				//////////////////////////////////////////////////////
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports; l++) {
					fprintf(fp, "wire [%d:0]%s_%s_%d;", \
						capacity*pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->num_pins-1,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->pb_type->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->placement_index);
					fprintf(fp, "//input to child\n");
				}
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports; l++) {
					fprintf(fp, "wire [%d:0]%s_%s_%d;", \
						capacity*pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->num_pins-1,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->pb_type->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->placement_index);
					fprintf(fp, "//clock to child\n");
				}
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports; l++) {
					fprintf(fp, "wire [%d:0]%s_%s_%d;", \
						capacity*pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->num_pins-1,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->pb_type->name,\
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->placement_index);
					fprintf(fp, "//output from child\n");
				}
				fprintf(fp, " \n");
			}
		}
	}
	fprintf(fp, "wire ground;\n\n"); //GND FOR UNUSED SIGNAL
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {

				dump_pb_pins_verilog(pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins,
					pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports, n_config_used, fp, mux_max_size);
				dump_pb_pins_verilog(pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins,
					pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports, n_config_used, fp, mux_max_size);
				fprintf(fp, " \n");

				//////////////////////////////////////////////////////
				// Instantiate children blocks
				//////////////////////////////////////////////////////
				flag_first = 0;
				fprintf(fp, "%s %s_%d (", 
					pb_graph_node->child_pb_graph_nodes[i][j][k].pb_type->name,
					pb_graph_node->child_pb_graph_nodes[i][j][k].pb_type->name, k);
				pb_graph_pins = pb_graph_node->clock_pins;
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports; l++) {
					if (flag_first == 0) flag_first = 1;
					else fprintf(fp, ",");					
					fprintf(fp, " .%s(%s_%s_%d)", 
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->name, 
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->pb_type->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->placement_index);
				}
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports; l++) {
					if (flag_first == 0) {flag_first = 1;}
					else fprintf(fp, ",");
					fprintf(fp, " .%s(%s_%s_%d)", 
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->name, 
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->pb_type->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->placement_index);
				}
				for (l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports; l++) {
					if (flag_first == 0) {flag_first = 1;}
					else fprintf(fp, ",");
					fprintf(fp, " .%s(%s_%s_%d)", 
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name, 
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->pb_type->name,
						pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->placement_index);
				}
				//determine if configuration bits are needed
				if (n_config_prev_per_child[i][j] != 0) { 
					int num_pb = pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
					fprintf(fp, ", .config_in(config_in[%d:%d]) ); //block instantiation\n", \
						n_config_used[0]+n_config_prev_per_child[i][j]/num_pb-1, n_config_used[0]);
					n_config_used[0] += n_config_prev_per_child[i][j]/num_pb;
				}
				else fprintf(fp, " );\n");
				fprintf(fp, "\n");
			}
		}
	}
	
	fprintf(fp, "//child output to block output\n");
	dump_pb_pins_verilog(pb_graph_node->output_pins,
		pb_graph_node->num_output_ports, n_config_used, fp, mux_max_size);
	fprintf(fp, "\n");
	
	fprintf(fp, "assign ground = 1'b0;\n");
	fprintf(fp, "\n");
	
	fprintf(fp, "endmodule\n\n\n");
	
	return n_config+n_config_prev_total;
}

void dump_pb_pins_verilog(INP t_pb_graph_pin **pb_graph_pins, INP int num_ports,
		INP int* n_config_used, INP FILE * fp, int* mux_max_size) {
	
	int i, j, k, m;
	int fan_in, mux_size;
	t_port *port, *edge_port;
	
	for (i = 0; i < num_ports; i++) {
		port = pb_graph_pins[i][0].port;
		for (j = 0; j < port->num_pins; j++) {
			assert(j == pb_graph_pins[i][j].pin_number);
			assert(pb_graph_pins[i][j].port == port);
			
			fan_in = pb_graph_pins[i][j].num_input_edges; 
			if (mux_max_size[0] < fan_in-1) mux_max_size[0] = fan_in;
			for (k = 0; k < fan_in; k++) {
				//first input edge, either create mux or use assign
				if (k == 0) { 
					if (fan_in > 1) {
						if (port->type == OUT_PORT) fprintf(fp, "mux%d mux_%d ( .in({", fan_in, n_config_used[0]);
						else fprintf(fp, "mux%d mux_%d ( .in({", fan_in+1, n_config_used[0]);
					} else if (port->type == OUT_PORT) {
						fprintf(fp, "assign %s[%d] = ",
							pb_graph_pins[i][j].port->name, j);
					} else {
						fprintf(fp, "assign %s_%s_%d[%d] = ",
							pb_graph_pins[i][j].port->name,
							pb_graph_pins[i][j].parent_node->pb_type->name,
							pb_graph_pins[i][j].parent_node->placement_index, j);
					}
				}
				for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {
					edge_port = pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port;
					if (edge_port->type == OUT_PORT || strcmp(pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->pb_type->name, pb_graph_pins[i][j].parent_node->pb_type->name)==0) {
						fprintf(fp, "%s_%s_%d[%d]",
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->pb_type->name,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->placement_index,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number);
					} else {
						fprintf(fp, "%s[%d]",
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number);
						
					}
				}
				//last input edge, close mux or assign
				if (k == fan_in-1) { 
					if (fan_in > 1) {
						if (port->type == OUT_PORT) {
							mux_size = (int)ceil(((double)log(fan_in)/log(2))); //no need for GND for outputs
							fprintf(fp, "}), .config_in(config_in[%d:%d]), .out(%s[%d]) );\n",
								mux_size+n_config_used[0]-1, n_config_used[0],
								pb_graph_pins[i][j].port->name, j);
							
						} else { //GND signal for unused pins, needed for config calculations and etc.
							mux_size = (int)ceil(((double)log(fan_in+1)/log(2))); //+1 because of GND
							fprintf(fp, ", ground}), .config_in(config_in[%d:%d]), .out(%s_%s_%d[%d]) );\n", 
								mux_size+n_config_used[0]-1, n_config_used[0],
								pb_graph_pins[i][j].port->name,
								pb_graph_pins[i][j].parent_node->pb_type->name,
								pb_graph_pins[i][j].parent_node->placement_index, j);
						}
						n_config_used[0] += mux_size;
					} else fprintf(fp, ";\n");
				} else fprintf(fp, ", ");
			}
		} 
	}
}

void dump_rr_graph_as_verilog (INP const char *file_name, int* pb_graph_config_bits, int* mux_max_size) {
	
	static const char *name_type[] = { "SOURCE", "SINK", "IPIN", "OPIN",
			"CHANX", "CHANY", "INTRA_CLUSTER_EDGE" };

	FILE *fp;
	FILE *fp_config;
	int inode, iconn;
	int iconfig = 0;
	
	t_rr_type rr_type;
	char* t_name;
	t_pb_graph_node *pb_graph_node;
	int cap, config_size, flag_first;
	char* config_array;
	int config_array_size = 0;
	
	fp = my_fopen(file_name, "w", 0);
	fp_config = my_fopen("bitgen_report.txt", "w", 0);
	
	int i, j, k, l;
	
	/////////////////////////////////////////////////////////////////////
	// create top module and top level inputs and outputs
	/////////////////////////////////////////////////////////////////////
	fprintf(fp, "module fpga_top (\n");
	fprintf(fp, "\tinput fpga_clk,\n");
	for (i=0; i<nx+2; i++) {
		for (j=0; j<ny+2; j++) {
			t_name = grid[i][j].type->name;
			cap = grid[i][j].type->capacity;
			if (strcmp(t_name, "io") == 0) {
				fprintf(fp, "\tinout [%d:0]%s_%d_%d_wire,\n", cap-1, t_name, i, j);
			}
		}
	}
	fprintf(fp, "\tinput config_reset,\n");
	fprintf(fp, "\tinput config_clk,\n");
	fprintf(fp, "\tinput config_in,\n");
	fprintf(fp, "\tinput config_en,\n");
	fprintf(fp, "\toutput config_out\n");
	fprintf(fp, ");\n\n");

	/////////////////////////////////////////////////////////////////////	
	// manage data for easy use
	//
	// 	node_fan_in[0...num_rr_nodes][0...fan_in]: Used to keep track of all
	//		the edges that connect to it (only the opposite information (fan_out) 
	//		is stored). This allows the top level mux creations.
	//
	//	node_fan_in_index[0...num_rr_nodes]: Used by node_fan_in to keep track of 
	// 		how many edges have been found currently.
	//
	//	node_trace: Used to keep track of all the paths
	// 		taken by the router in the top level mux.
	//		node_trace[0 ... num_rr_nodes][2]
	//			node_trace[i][0] = prev_node
	//			node_trace[i][1] = net_num
	//
	//	x_pin_sort_by_grid[0...nx+2][0...ny+2][0...num_x_ports][0...num_x_pins]: 
	//		used to assign correct rr_graph nodes to the clb inputs.
	/////////////////////////////////////////////////////////////////////
	int***** clock_pin_sort_by_grid = (int*****)malloc((nx+2)*sizeof(int****));
	int***** input_pin_sort_by_grid = (int*****)malloc((nx+2)*sizeof(int****));
	int***** output_pin_sort_by_grid = (int*****)malloc((nx+2)*sizeof(int****));
	
	int** node_fan_in = (int**)malloc((num_rr_nodes)*sizeof(int*));
	int* node_fan_in_index = (int*)malloc((num_rr_nodes)*sizeof(int));
	
	int* node_trace = (int*)malloc(num_rr_nodes*sizeof(int));
	int* node_net_num = (int*)malloc(num_rr_nodes*sizeof(int));
	
	for (i=0; i<nx+2; i++) {
		clock_pin_sort_by_grid[i] = (int****)malloc((ny+2)*sizeof(int***));
		input_pin_sort_by_grid[i] = (int****)malloc((ny+2)*sizeof(int***));
		output_pin_sort_by_grid[i] = (int****)malloc((ny+2)*sizeof(int***));
		
		for (j=0; j<ny+2; j++) {
			pb_graph_node = grid[i][j].type->pb_graph_head;
			t_name = grid[i][j].type->name;
			if (strcmp(t_name, "<EMPTY>") == 0) continue;
			
			clock_pin_sort_by_grid[i][j] = (int***)malloc(grid[i][j].type->capacity*sizeof(int**));
			input_pin_sort_by_grid[i][j] = (int***)malloc(grid[i][j].type->capacity*sizeof(int**));
			output_pin_sort_by_grid[i][j] = (int***)malloc(grid[i][j].type->capacity*sizeof(int**));
			
			for (k=0; k<grid[i][j].type->capacity; k++) {
				clock_pin_sort_by_grid[i][j][k] = (int**)malloc(pb_graph_node->num_clock_ports*sizeof(int*));
				input_pin_sort_by_grid[i][j][k] = (int**)malloc(pb_graph_node->num_input_ports*sizeof(int*));
				output_pin_sort_by_grid[i][j][k] = (int**)malloc(pb_graph_node->num_output_ports*sizeof(int*));
				
				for (l=0; l<pb_graph_node->num_clock_ports; l++)
					clock_pin_sort_by_grid[i][j][k][l] = (int*)malloc(pb_graph_node->num_clock_pins[l]*sizeof(int));
				for (l=0; l<pb_graph_node->num_input_ports; l++)
					input_pin_sort_by_grid[i][j][k][l] = (int*)malloc(pb_graph_node->num_input_pins[l]*sizeof(int));
				for (l=0; l<pb_graph_node->num_output_ports; l++)
					output_pin_sort_by_grid[i][j][k][l] = (int*)malloc(pb_graph_node->num_output_pins[l]*sizeof(int));
			}
		}
	}

	
	t_port *port;
	t_pb_graph_pin *current_pin;
	
	for (inode = 0; inode < num_rr_nodes; inode++) {
		i = rr_node[inode].xlow;
		j = rr_node[inode].ylow;
		k = rr_node[inode].z;
		rr_type = rr_node[inode].type;
		
		//given the pb_graph_node and the ptc_num, pin info is provided
		current_pin = get_pb_graph_node_pin_from_pb_graph_node\
			(grid[i][j].type->pb_graph_head, rr_node[inode].ptc_num);
		port = current_pin->port;
		
		//only the IPIN and OPINs are relevant to pb_graph_node pins
		if (name_type[rr_type] == name_type[2] || name_type[rr_type] == name_type[3]) {	
			if (port->is_clock)
				clock_pin_sort_by_grid[i][j][k][port->port_index_by_type][current_pin->pin_number] = inode;
			else if (port->type == IN_PORT) {
				input_pin_sort_by_grid[i][j][k][port->port_index_by_type][current_pin->pin_number] = inode;
			} else
				output_pin_sort_by_grid[i][j][k][port->port_index_by_type][current_pin->pin_number] = inode;
		}
		
		//initialize node_fan_in
		node_fan_in[inode] = (int*)malloc((rr_node[inode].fan_in)*sizeof(int));
		node_fan_in_index[inode] = rr_node[inode].fan_in;
		
		node_net_num[inode] = -1;
		
	}
	
	for (inode = 0; inode < num_rr_nodes; inode++) {
		for (iconn=0; iconn<rr_node[inode].num_edges; iconn++) {
			node_fan_in[rr_node[inode].edges[iconn]][node_fan_in_index[rr_node[inode].edges[iconn]]-1]=inode;
			node_fan_in_index[rr_node[inode].edges[iconn]]--;
		}
	}

	struct s_trace *tptr;
	for (i=0; i<num_nets; i++) {
		tptr = trace_head[i];
		while (tptr != NULL && tptr->next != NULL) {
			//printf("%d -> ", tptr->index);
			rr_type = rr_node[tptr->index].type;
			if (name_type[rr_type] != name_type[1]) {				
				node_trace[tptr->next->index] = tptr->index;
				node_net_num[tptr->next->index] = clb_to_vpack_net_mapping[i]; //match nets with clb
			}
			tptr = tptr->next;
		}
		if (tptr != NULL && tptr->next == NULL) {
			//printf("%d.", tptr->index);
		} else if (tptr == NULL) {
			//printf("empty");
		}	
		//printf("\n");
	}
	
	/////////////////////////////////////////////////////////////////////	
	// create connecting wires
	/////////////////////////////////////////////////////////////////////
	for (inode = 0; inode < num_rr_nodes; inode++) {
		rr_type = rr_node[inode].type;
		if (name_type[rr_type] != name_type[0] && name_type[rr_type] != name_type[1]) {
			fprintf(fp, "wire n%d; //%s (%d,%d) #%d", inode, name_type[rr_type], \
				rr_node[inode].xlow, rr_node[inode].ylow, rr_node[inode].ptc_num);
			for (iconn = 0; iconn < rr_node[inode].num_edges; iconn++) {
				fprintf(fp, " %d", rr_node[inode].edges[iconn]);
			}
			fprintf(fp, "\n");
		}
	} 
	fprintf(fp, "\n\n");
	
	fprintf(fp, "wire [%d:0] config_chain;\n", iconfig-1);
	
	/////////////////////////////////////////////////////////////////////	
	//create channels
	/////////////////////////////////////////////////////////////////////	
	int c_mux = 0;
	int mux_size = 0;
	int config_value = 0;
	for (inode = 0; inode < num_rr_nodes; inode++) {
		rr_type = rr_node[inode].type;
		mux_size = rr_node[inode].fan_in;
		config_size = (int)ceil((double)log(mux_size)/log(2));
		if ((name_type[rr_type] != name_type[0] && name_type[rr_type] != name_type[1]) && mux_size > 1) {
			fprintf(fp, "mux%d mux_%d (.in({", mux_size, c_mux);
			for (iconn=0; iconn<mux_size-1; iconn++) {
				if(node_trace[inode]==node_fan_in[inode][iconn]) {
					fprintf(fp, "/**/");
					config_value = mux_size-iconn-1;
				}
				fprintf(fp, "n%d, ", node_fan_in[inode][iconn]);	
			}
			if(node_trace[inode]==node_fan_in[inode][iconn]) {
				fprintf(fp, "/**/");
				config_value = mux_size-iconn-1;
			}
			fprintf(fp, "n%d", node_fan_in[inode][iconn]);	
			fprintf(fp, "}), .out(n%d), .config_in(config_chain[%d:%d])); \n", inode, iconfig+config_size-1, iconfig);
			fprintf(fp_config, "[%d:%d] (mux_%d) %d\n", iconfig+config_size-1, iconfig, c_mux, config_value);
			config_set(iconfig+config_size-1, iconfig, config_value, &config_array, &config_array_size, fp_config);
			c_mux++;
			iconfig += config_size;
			if (mux_max_size[0] < mux_size) 
				mux_max_size[0] = mux_size;
 		}
	}
	fprintf(fp, "\n");
	
	for (i=0; i<num_rr_nodes; i++) {
		free(node_fan_in[i]);
	}
	free(node_fan_in);
	free(node_fan_in_index);

	/////////////////////////////////////////////////////////////////////	
	// connect up the blocks
	// 	first, create wires with block_x_y naming for easy debugging
	//	then, connect the nodes that correspond to those and assign
	/////////////////////////////////////////////////////////////////////
	int icap;
	for (i=0; i<nx+2; i++) {
		for (j=0; j<ny+2; j++) {
			pb_graph_node = grid[i][j].type->pb_graph_head;
			t_name = grid[i][j].type->name;
			cap = grid[i][j].type->capacity;
			config_size = pb_graph_config_bits[grid[i][j].type->index];
			if (strcmp(t_name, "<EMPTY>") == 0) continue;
		
			int*** input_equivalence_net_order[3];
			
			//create wires to connect to the blocks
			//makes it easier for debugging
			if (strcmp(t_name, "io")!=0) { //CLOCK for IO means nothing currently!!
				flag_first = 0;
				input_equivalence_net_order[0] = (int***)malloc(pb_graph_node->num_clock_ports*sizeof(int**));

				for (k=0; k<pb_graph_node->num_clock_ports; k++) {
					input_equivalence_net_order[0][k] = (int**)malloc(pb_graph_node->num_clock_pins[k]*sizeof(int*));
					fprintf(fp, "wire [%d:0]%s_%d_%d;\n", \
						cap*pb_graph_node->pb_type->num_clock_pins-1,\
						pb_graph_node->clock_pins[k][0].port->name, i, j);
					for (l=pb_graph_node->num_clock_pins[k]-1; l>=0; l--) {
						input_equivalence_net_order[0][k][l] = (int*)malloc(cap*sizeof(int));
						for (icap = cap-1; icap>=0; icap--) {
							input_equivalence_net_order[0][k][l][icap] = node_net_num[clock_pin_sort_by_grid[i][j][icap][k][l]];
							if (flag_first == 0) {
								fprintf(fp, "assign %s_%d_%d = {fpga_clk", \
									pb_graph_node->clock_pins[0][0].port->name, i, j);
								flag_first = 1;
							} else {
								fprintf(fp, ", fpga_clk");
							}/*
							if (flag_first == 0) {
								fprintf(fp, "assign %s_%d_%d = {n%d", \
									pb_graph_node->clock_pins[k][l].port->name, i, j, \
									clock_pin_sort_by_grid[i][j][icap][k][l]);
								flag_first = 1;
							} else {
								fprintf(fp, ", n%d", clock_pin_sort_by_grid[i][j][icap][k][l]);
							}*/
						}
					}
				}
				fprintf(fp, "};\n");				
			}
			
			flag_first = 0;
			input_equivalence_net_order[1] = (int***)malloc(pb_graph_node->num_input_ports*sizeof(int**));
			for (k=0; k<pb_graph_node->num_input_ports; k++) {
				input_equivalence_net_order[1][k] = (int**)malloc(pb_graph_node->num_input_pins[k]*sizeof(int*));
				fprintf(fp, "wire [%d:0]%s_%d_%d;\n", \
					cap*pb_graph_node->pb_type->num_input_pins-1, \
					pb_graph_node->input_pins[k][0].port->name, i, j);
				for (l=pb_graph_node->num_input_pins[k]-1; l>=0; l--) {
					input_equivalence_net_order[1][k][l] = (int*)malloc(cap*sizeof(int));
					for (icap = cap-1; icap>=0; icap--) {
						input_equivalence_net_order[1][k][l][icap] = node_net_num[input_pin_sort_by_grid[i][j][icap][k][l]];
						if (flag_first == 0) {
							fprintf(fp, "assign %s_%d_%d = {n%d", \
								pb_graph_node->input_pins[k][l].port->name, i, j, \
								input_pin_sort_by_grid[i][j][icap][k][l]);
							flag_first = 1;
						} else {
							fprintf(fp, ", n%d", input_pin_sort_by_grid[i][j][icap][k][l]);
						}	
					}
				}
			}
			fprintf(fp, "};\n");
			
			flag_first = 0;
			input_equivalence_net_order[2] = (int***)malloc(pb_graph_node->num_output_ports*sizeof(int**));
			for (k=0; k<pb_graph_node->num_output_ports; k++) {
				input_equivalence_net_order[2][k] = (int**)malloc(pb_graph_node->num_output_pins[k]*sizeof(int*));
				fprintf(fp, "wire [%d:0]%s_%d_%d;\n", \
					cap*pb_graph_node->pb_type->num_output_pins-1, \
					pb_graph_node->output_pins[k][0].port->name, i, j);
				for (l=pb_graph_node->num_output_pins[k]-1; l>=0; l--) {
					input_equivalence_net_order[2][k][l] = (int*)malloc(cap*sizeof(int));
					for (icap = cap-1; icap>=0; icap--) {
						input_equivalence_net_order[2][k][l][icap] = node_net_num[output_pin_sort_by_grid[i][j][icap][k][l]];
						if (flag_first == 0) {
							fprintf(fp, "assign {n%d", output_pin_sort_by_grid[i][j][icap][k][l]);
							flag_first = 1;
						} else {
							fprintf(fp, ", n%d", output_pin_sort_by_grid[i][j][icap][k][l]);
						}
					}
				}
				fprintf(fp, "} = %s_%d_%d;\n", pb_graph_node->output_pins[k][l+1].port->name, i, j);
			}
			fprintf(fp, "\n");
			
			//debugging
			/*if (strcmp(t_name, "io")!=0) { //CLOCK for IO means nothing currently!!
				for (k=0; k<pb_graph_node->num_clock_ports; k++) {
					for (l=pb_graph_node->num_clock_pins[k]-1; l>=0; l--) {
						for (icap = 0; icap<cap; icap++) 
						printf("clock [%d][%d][%d][%d]%d\n", i, j, k, l, input_equivalence_net_order[0][k][l][icap]);
					}
				}	
			}
			for (k=0; k<pb_graph_node->num_input_ports; k++) {
				for (l=pb_graph_node->num_input_pins[k]-1; l>=0; l--) {
						for (icap = 0; icap<cap; icap++) 
					printf("input [%d][%d][%d][%d]%d\n", i, j, k, l, input_equivalence_net_order[1][k][l][icap]);
				}
			}	
			for (k=0; k<pb_graph_node->num_output_ports; k++) {
				for (l=pb_graph_node->num_output_pins[k]-1; l>=0; l--) {
						for (icap = 0; icap<cap; icap++) 
					printf("output [%d][%d][%d][%d]%d\n", i, j, k, l, input_equivalence_net_order[2][k][l][icap]);
				}
			}printf("\n");*/
			
			//Block instantiation
			flag_first = 0;
			fprintf(fp, "%s %s_%d_%d ( ", t_name, t_name, i, j);
			for (k=0; k<pb_graph_node->num_input_ports; k++) {
				if (flag_first == 0) flag_first = 1;
				else fprintf(fp, ", ");
				fprintf(fp, ".%s(%s_%d_%d)", \
					pb_graph_node->input_pins[k][0].port->name, \
					pb_graph_node->input_pins[k][0].port->name, i, j);
			}
			for (k=0; k<pb_graph_node->num_output_ports; k++) {
				if (flag_first == 0) flag_first = 1;
				else fprintf(fp, ", ");
				fprintf(fp, ".%s(%s_%d_%d)", \
					pb_graph_node->output_pins[k][0].port->name, \
					pb_graph_node->output_pins[k][0].port->name, i, j);
			}
			if (strcmp(t_name, "io")!=0) {
				//CLOCK for IO means nothing currently!!
				for (k=0; k<pb_graph_node->num_clock_ports; k++) {
					if (flag_first == 0) flag_first = 1;
					else fprintf(fp, ", ");
					fprintf(fp, ".%s(%s_%d_%d)",\
						pb_graph_node->clock_pins[k][0].port->name, \
						pb_graph_node->clock_pins[k][0].port->name, i, j);
				}
			}				
			if (config_size !=0 && strcmp(t_name, "io")!=0) {
				fprintf(fp, ", .config_in(config_chain[%d:%d]) );\n\n",\
					iconfig+config_size-1, iconfig);
			} else if (strcmp(t_name, "io")==0) {
				config_size = cap;
				//External connection here
				fprintf(fp, ", .%s_ext(%s_%d_%d_wire), .config_in(config_chain[%d:%d]) );\n\n",\
					t_name, t_name, i, j,
					iconfig+config_size-1, iconfig);
			} else {
				fprintf(fp, " );\n\n");
			}
			config_set_block (iconfig+config_size-1, iconfig, \
				grid[i][j].blocks, grid[i][j].type->capacity, input_equivalence_net_order, \
				&config_array, &config_array_size, fp_config);
			iconfig+=config_size;
		}
	}
	fprintf(fp, "\n");

	/////////////////////////////////////////////////////////////////////	
	// block instantiation
	// - io has special treatment (no clock + external connection)
	/////////////////////////////////////////////////////////////////////
	/*for (i=0; i<nx+2; i++) {
		for (j=0; j<ny+2; j++) {
			pb_graph_node = grid[i][j].type->pb_graph_head;
			t_name = grid[i][j].type->name;
			cap = grid[i][j].type->capacity;
			config_size = pb_graph_config_bits[grid[i][j].type->index];
			if (strcmp(t_name, "<EMPTY>") != 0) {

				flag_first = 0;
				fprintf(fp, "%s %s_%d_%d ( ", t_name, t_name, i, j);
				for (k=0; k<pb_graph_node->num_input_ports; k++) {
					if (flag_first == 0) flag_first = 1;
					else fprintf(fp, ", ");
					fprintf(fp, ".%s(%s_%d_%d)", \
						pb_graph_node->input_pins[k][0].port->name, \
						pb_graph_node->input_pins[k][0].port->name, i, j);
				}
				for (k=0; k<pb_graph_node->num_output_ports; k++) {
					if (flag_first == 0) flag_first = 1;
					else fprintf(fp, ", ");
					fprintf(fp, ".%s(%s_%d_%d)", \
						pb_graph_node->output_pins[k][0].port->name, \
						pb_graph_node->output_pins[k][0].port->name, i, j);
				}
				if (strcmp(t_name, "io")!=0) {
					//CLOCK for IO means nothing currently!!
					for (k=0; k<pb_graph_node->num_clock_ports; k++) {
						if (flag_first == 0) flag_first = 1;
						else fprintf(fp, ", ");
						fprintf(fp, ".%s(%s_%d_%d)",\
							pb_graph_node->clock_pins[k][0].port->name, \
							pb_graph_node->clock_pins[k][0].port->name, i, j);
					}
				}				
				if (config_size !=0 && strcmp(t_name, "io")!=0) {
					fprintf(fp, ", .config_in(config_chain[%d:%d]) );\n\n",\
						iconfig+config_size-1, iconfig);
				} else if (strcmp(t_name, "io")==0) {
					config_size = cap;
					//External connection here
					fprintf(fp, ", .%s_ext(%s_%d_%d_wire), .config_in(config_chain[%d:%d]) );\n\n",\
						t_name, t_name, i, j,
						iconfig+config_size-1, iconfig);
				} else {
					fprintf(fp, " );\n\n");
				}
				
				config_set_block (iconfig+config_size-1, iconfig, \
					grid[i][j].blocks, grid[i][j].type->capacity, \
					&config_array, &config_array_size, fp_config);
				iconfig+=config_size;
			}
		}
	}*/

	/////////////////////////////////////////////////////////////////////	
	// config stuff comment later
	// - io has special treatment (no clock + external connection)
	/////////////////////////////////////////////////////////////////////
	
	fprintf(fp, "wire [%d:0]config_chain_shift;\n", iconfig-1);
	fprintf(fp, "assign config_chain = config_en?0:config_chain_shift;\n");
	
	fprintf(fp, "config_helper config_helper_0 ( .config_in(config_in), ");
	fprintf(fp, ".config_out(config_chain_shift[%d:0]), .config_en(config_en), .reset(config_reset), .clk(config_clk) );\n", iconfig-1);
	fprintf(fp, "\n");
	
	fprintf(fp, "endmodule\n\n");
	
	fprintf(fp, "module config_helper (\n");
	fprintf(fp, "\tinput clk,\n");
	fprintf(fp, "\tinput reset,\n");
	fprintf(fp, "\tinput config_in,\n");
	fprintf(fp, "\tinput config_en,\n");
	fprintf(fp, "\toutput [%d:0]config_out\n", iconfig-1);
	fprintf(fp, ");\n\n");
	

	fprintf(fp, "latch config_latch_start ( .clk(clk), .reset(reset), .en(config_en), .in(config_in), .out(config_out[0]) );\n");
	fprintf(fp, "generate\ngenvar i;\nfor(i=0; i<%d; i=i+2)\nbegin: shifter1\n", iconfig);
	fprintf(fp, "latch config_latch ( .clk(clk), .reset(reset), .en(config_en), .in(config_out[i]), .out(config_out[i+1]) );\nend\nendgenerate\n\n");
	fprintf(fp, "generate\nfor(i=1; i<%d; i=i+2)\nbegin: shifter2\n", iconfig-1);
	fprintf(fp, "latch config_latch ( .clk(clk), .reset(reset), .en(config_en), .in(config_out[i]), .out(config_out[i+1]) );\nend\nendgenerate\n\n");

	fprintf(fp, "endmodule\n");
	
	for (i=0; i<config_array_size%8; i++) {
		config_set(config_array_size, config_array_size, 0, &config_array, &config_array_size, fp_config);
	}
	
	fprintf(fp_config, "\nbitstream [%d:0]\n", iconfig-1);
	for (i=0; i<ceil((double)config_array_size/8); i++) {
		for (j=7; j>=0; j--) {
			if (i*8+(8-j-1) > iconfig-1) break;
			fprintf(fp_config, "%d", (config_array[i]>>j)&1);
		}
	}
	
	//USE BELOW CODE TO DEBUG BITSTREAM
	/*fprintf(fp_config, "\n");
	for (i=0; i<ceil((double)config_array_size/8); i++) {
		for (j=7; j>=0; j--) {
			if (i*8+(8-j-1) > iconfig-1) break;
			fprintf(fp_config, "[%d]%d\n", (i*8)+(8-j-1), (config_array[i]>>j)&1);
		}
	}
	fprintf(fp_config, "\n");*/
	
	fclose(fp);
	

}

t_pb_graph_pin* get_pb_graph_node_pin_from_pb_graph_node(t_pb_graph_node *pb_graph_node, int ipin) {
	int i, count;
	const t_pb_type *pb_type;
	pb_type = pb_graph_node->pb_type;

	/* If this is post-placed, then the ipin may have been shuffled up by the z * num_pins, 
	bring it back down to 0..num_pins-1 range for easier analysis */
	ipin %= (pb_type->num_input_pins + pb_type->num_output_pins + pb_type->num_clock_pins);
		
	if(ipin < pb_type->num_input_pins) {
		count = ipin;
		for(i = 0; i < pb_graph_node->num_input_ports; i++) {
			if(count - pb_graph_node->num_input_pins[i] >= 0) {
				count -= pb_graph_node->num_input_pins[i];
			} else {
				return &pb_graph_node->input_pins[i][count];
			}
		}
	} else if (ipin < pb_type->num_input_pins + pb_type->num_output_pins) {
		count = ipin - pb_type->num_input_pins;
		for(i = 0; i < pb_graph_node->num_output_ports; i++) {
			if(count - pb_graph_node->num_output_pins[i] >= 0) {
				count -= pb_graph_node->num_output_pins[i];
			} else {
				return &pb_graph_node->output_pins[i][count];
			}
		}
	} else {
		count = ipin - pb_type->num_input_pins - pb_type->num_output_pins;
		for(i = 0; i < pb_graph_node->num_clock_ports; i++) {
			if(count - pb_graph_node->num_clock_pins[i] >= 0) {
				count -= pb_graph_node->num_clock_pins[i];
			} else {
				return &pb_graph_node->clock_pins[i][count];
			}
		}
	}
	assert(0);
	return NULL;
}

