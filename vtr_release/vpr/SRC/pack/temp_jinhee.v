
int echo_pb_rec_verilog (const INP t_pb_graph_node *pb_graph_node, INP int level, INP FILE *fp, INP int cap) {

	int i, j, k;
		
	t_pb_graph_pin **pb_graph_pins;
	t_port *port;

	//////////////////////////////////////////////////////
	// Recursively go down to bottom of list (lut and ff)
	//////////////////////////////////////////////////////
	//pass back configuration bits to add to current level
	int n_config_prev = 0; 
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			n_config_prev+=pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb*echo_pb_rec_verilog(&pb_graph_node->child_pb_graph_nodes[i][j][0], level, fp, cap);
		}	
	}
	
	//////////////////////////////////////////////////////
	// Figure out config bit count
	// - repeat but need this info before module declaration
	//////////////////////////////////////////////////////
	int n_config = 0;
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				if (strcmp(pb_graph_node->child_pb_graph_nodes[i][j][k].pb_type->name, "lut")==0) {n_config +=64;}			

				for (int l = 0; l < pb_graph_node->num_input_ports; l++) {
					port = pb_graph_node->input_pins[l][0].port;		
					for (int h = 0; h < port->num_pins; h++) {
						if (pb_graph_node->input_pins[l][h].num_input_edges > 1) {
							n_config +=(int)ceil(((double)log(pb_graph_node->input_pins[l][h].num_input_edges)/log(2)));
						}
					}
				}

				for (int l = 0; l < pb_graph_node->num_clock_ports; l++) {
					port = pb_graph_node->clock_pins[l][0].port;		
					for (int h = 0; h < port->num_pins; h++) {
						if (pb_graph_node->clock_pins[l][h].num_input_edges > 1) {
							n_config +=(int)ceil(((double)log(pb_graph_node->clock_pins[l][h].num_input_edges)/log(2)));
						}
					}
				}

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
	pb_graph_pins = pb_graph_node->input_pins;
	int temp_port_count = pb_graph_node->num_input_ports + pb_graph_node->num_output_ports + pb_graph_node->num_clock_ports-1;
	if (n_config_prev != 0) {
		fprintf(fp, "\tinput [%d:0]config_in,\n", (n_config_prev)-1);	
		fprintf(fp, "\toutput config_out, \n");	
	}
	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\tinput [%d:0]%s", cap*port->num_pins-1, port->name);
		if (temp_port_count != i) fprintf(fp, ",\n");
	}
	pb_graph_pins = pb_graph_node->output_pins;
	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\toutput [%d:0]%s", cap*port->num_pins-1, port->name);
		if (temp_port_count != pb_graph_node->num_input_ports+i) fprintf(fp, ",\n");
	}
	pb_graph_pins = pb_graph_node->clock_pins;
	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		port = pb_graph_pins[i][0].port;
		fprintf(fp, "\tinput [%d:0]%s", cap*port->num_pins-1, port->name);
		if (temp_port_count != pb_graph_node->num_input_ports+pb_graph_node->num_output_ports+i) fprintf(fp, ",\n");
	}
	fprintf(fp, "\n);\n\n");
	
	//////////////////////////////////////////////////////
	// Inside of the module
	//////////////////////////////////////////////////////
	int flag_first = 0;
	int n_child = 0;
	int n_config = 0;
	
	//go through all the immediate children
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				fprintf(fp, "////////CHILD %d////////\n", n_child); n_child++;
				
				//connect child input/clock/output pins
				fprintf(fp, "//input ports\n");
				n_config +=echo_pb_pins_verilog(
					pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins,
					pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports, 1, fp);
				fprintf(fp, "//clock ports\n");
				n_config +=echo_pb_pins_verilog(pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins,
					pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports, 1, fp);
				fprintf(fp, "//output ports\n");
				if (pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports > 0) {
					for (int l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports; l++) {
						fprintf(fp, "wire [%d:0]%s_%s_%d;\n", \
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->num_pins-1,\
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name,\
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->pb_type->name,\
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->placement_index);
						}
				}
				
				//instantiate block
				fprintf(fp, "//block instantiation\n");
				flag_first = 0;
				fprintf(fp, "%s %s_%d (", 
					pb_graph_node->child_pb_graph_nodes[i][j][k].pb_type->name,
					pb_graph_node->child_pb_graph_nodes[i][j][k].pb_type->name,	k);
				pb_graph_pins = pb_graph_node->clock_pins;
				for (int l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports; l++) {
					if (pb_graph_node->child_pb_graph_nodes[i][j][k].num_clock_ports > 0) {
						if (flag_first == 0) {flag_first = 1;}
						else fprintf(fp, ",");					
						fprintf(fp, " .%s(%s_%s_%d)", 
							pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->name, 
							pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].port->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->pb_type->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].clock_pins[l][0].parent_node->placement_index);
					}
				}
				for (int l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports; l++) {
					if (pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports > 0) {
						if (flag_first == 0) {flag_first = 1;}
						else fprintf(fp, ",");
						fprintf(fp, " .%s(%s_%s_%d)", 
							pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->name, 
							pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].port->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->pb_type->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins[l][0].parent_node->placement_index);
					}
				}
				for (int l = 0;  l< pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports; l++) {				
					if (pb_graph_node->child_pb_graph_nodes[i][j][k].num_output_ports > 0) {
						if (flag_first == 0) {flag_first = 1;}
						else fprintf(fp, ",");
						fprintf(fp, " .%s(%s_%s_%d)", 
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name, 
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].port->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->pb_type->name,
							pb_graph_node->child_pb_graph_nodes[i][j][k].output_pins[l][0].parent_node->placement_index);
					}
					fprintf(fp, ".config_in(config_in[%d:%d]), .config_out(config_out) );\n", 1,1);
				}
				fprintf(fp, "\n\n");
			}
		}
	}
	
	fprintf(fp, "//child output connected to block output\n");
	n_config +=echo_pb_pins_verilog(pb_graph_node->output_pins,
		pb_graph_node->num_output_ports, 0, fp);
	
	fprintf(fp, "\n");
	fprintf(fp, "endmodule\n\n\n");
	
	return n_config+n_config_prev;
}

static int echo_pb_pins_verilog(INP t_pb_graph_pin **pb_graph_pins, INP int num_ports,
		INP int level, INP FILE * fp) {
	int i, j, k, m;
	int n_config=0;
	t_port *port;

	for (i = 0; i < num_ports; i++) {
		port = pb_graph_pins[i][0].port;
		
		if (port->type == OUT_PORT) {
		
			for (j = 0; j < port->num_pins; j++) {
				assert(j == pb_graph_pins[i][j].pin_number);
				assert(pb_graph_pins[i][j].port == port);
				
				for (k = 0; k < pb_graph_pins[i][j].num_input_edges; k++) {
					if (k == 0) {
						if (pb_graph_pins[i][j].num_input_edges > 1) {
							n_config += (int)ceil(((double)log(pb_graph_pins[i][j].num_input_edges)/log(2)));
							fprintf(fp, "mux%d mux_%d ( .in({", n_config, j);
						} else {
							fprintf(fp, "assign %s[%d] = ",
								pb_graph_pins[i][j].port->name, j);
						}
					}
					for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {
						fprintf(fp, "%s_%s_%d[%d]",
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->pb_type->name,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->placement_index,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number);
					}
					if (k == pb_graph_pins[i][j].num_input_edges-1) {
						if (pb_graph_pins[i][j].num_input_edges > 1) fprintf(fp, "}), .out(%s[%d]) );\n",
							pb_graph_pins[i][j].port->name, j);
						else fprintf(fp, ";\n");
					} else fprintf(fp, ", ");
				}
			}
		} else {
			fprintf(fp, "wire [%d:0]%s_%s_%d;\n", port->num_pins-1,
				pb_graph_pins[i][0].port->name,
				pb_graph_pins[i][0].parent_node->pb_type->name,
				pb_graph_pins[i][0].parent_node->placement_index);
		
			for (j = 0; j < port->num_pins; j++) {
				assert(j == pb_graph_pins[i][j].pin_number);
				assert(pb_graph_pins[i][j].port == port);
			

				for (k = 0; k < pb_graph_pins[i][j].num_input_edges; k++) {
			
					if (k == 0) {
						if (pb_graph_pins[i][j].num_input_edges > 1) {
							n_config += (int)ceil(((double)log(pb_graph_pins[i][j].num_input_edges)/log(2)));
							fprintf(fp, "mux%d mux_%d ( .in({", n_config, j);
						} else {
							fprintf(fp, "assign %s_%s_%d[%d] = ",
								pb_graph_pins[i][j].port->name,
								pb_graph_pins[i][j].parent_node->pb_type->name,
								pb_graph_pins[i][j].parent_node->placement_index, j);
						}
					}
					for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {
						if (strcmp(pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->pb_type->name, pb_graph_pins[i][j].parent_node->pb_type->name)!=0)
							fprintf(fp, "%s[%d]",
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name,
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number);
						else 
							fprintf(fp, "%s_%s_%d[%d]",
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name,
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->pb_type->name,
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->parent_node->placement_index,
								pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number);
					}
					if (k == pb_graph_pins[i][j].num_input_edges-1) {
						if (pb_graph_pins[i][j].num_input_edges > 1) fprintf(fp, "}), .out(%s_%s_%d[%d]) );\n",
							pb_graph_pins[i][j].port->name,
							pb_graph_pins[i][j].parent_node->pb_type->name,
							pb_graph_pins[i][j].parent_node->placement_index, j);
						else fprintf(fp, ";\n");
					} else fprintf(fp, ", ");
				}

			}
		
		}
	}
	return n_config;
}

