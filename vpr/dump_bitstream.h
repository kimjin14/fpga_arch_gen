/*
This function takes the range of configuration bits and the value to update the array.
It will always realloc array to fit the new value and update the array size.
	high/low: the range of config bits (wire [high:low]config)
	value: the value of what the config will be
	config_array: array containing the configuration bits
	config_array_size: size of config_array
	
USE: when value is in integer and can be stored in an int (high-low < sizeof(int))*/
void config_set(int high, int low, int value, \
	char** config_array, int* config_array_size, FILE * fp);

/*
This function sets the configuration bits of logical_blocks.
It takes the set range [high:low] of the block and sets the values.
If I/O, sets config bit to 0 if output and 1 if input.
Else, looks through children in **SAME ORDER** as architecture gen and sets values.
	high/low: the range of config bits (wire [high:low]config)
	logical_blk_indices: array of logical_block indices to configure
	logical_blk_cap: capacity of the block (I/O can fit 8 in a grid)
	config_array: array containing the configuration bits
	config_array_size: size of config_array
	fp: for debugging purposes (prints to bitgen_report.txt)
	
USE: for a grid coordinate where pb types have many children*/
void config_set_block (int high, int low, \
	int* logical_blk_indices, int logical_blk_cap, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp);

/*
This function recursively sets the config bits of the children.
It takes the pb and starting configuration bit to set values.
	pb: physical block to be configured
	low_config: current state of config, high is determined through fan_in
	config_array: array containing the configuration bits
	config_array_size: size of config_array
	fp: for debugging purposes (prints to bitgen_report.txt)
	
USE: pb_types used for architecture generation*/
void config_set_block_rec (t_pb *pb, int* low_config, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp);

/*
This function looks at all the input edges of the pin and
determines which edge is used (by comparing to rr_graph).
	pb_graph_pins: pins for configuring
	num_ports: number of ports for the pin
	low_config: current state of config, high is determined through fan_in
	rr_graph_of_cluster: rr_graph of the cluster for matching input edges
	config_array: array containing the configuration bits
	config_array_size: size of config_array
	fp: for debugging purposes (prints to bitgen_report.txt)
	
USE: pins of pb_types visted in the order of arch gen*/	
void config_set_pins(t_pb_graph_pin **pb_graph_pins, int num_ports, \
	int* low_config, s_rr_node *rr_graph_of_cluster, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp);
	

/* This function prints the bitstream for the LUT truth table.
Uses load_table from verilog_writer.c.
	pb: physical block containing the truth table
	low_config: current state of config, high is determined through fan_in
	config_array: array containing the configuration bits
	config_array_size: size of config_array
	fp: for debugging purposes (prints to bitgen_report.txt

USE: for pb primitive LUT only!!*/
void config_set_LUT(t_pb *pb, int* low_config, \
	char** config_array, int* config_array_size, FILE * fp);
	



void config_set_empty_pins(t_pb_graph_pin **pb_graph_pins, int num_ports, \
	int* low_config, char** config_array, int* config_array_size, FILE * fp);
void config_set_empty_block_rec (t_pb_graph_node *pb_graph_node, int* low_config, 
	char** config_array, int* config_array_size, FILE * fp);
	
char *load_truth_table_without_pb(int inputs , s_linked_vptr *truth_table);



