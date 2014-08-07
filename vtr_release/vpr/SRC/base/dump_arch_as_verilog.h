void dump_arch_as_verilog (void);
void dump_pb_graph_as_verilog (INP const char *file_name, int* pb_graph_config_bits, int* mux_max_size);
void dump_rr_graph_as_verilog (INP const char *file_name, int* pb_graph_config_bits, int* mux_max_size);
void dump_mux_as_verilog (INP const char *file_name, int* mux_max_size);
void config_set_block (INP const char *file_name, int high, int low, int* blk_indices, \
		int cap, int** config_array, int* config_array_size);
void config_set(int high, int low, int value, int** config_array, int* config_array_size);
int dump_pb_rec_verilog (const INP t_pb_graph_node *pb_graph_node, INP FILE *fp, \
		INP int capacity, int* mux_max_size);
void dump_pb_pins_verilog(INP t_pb_graph_pin **pb_graph_pins, INP int num_ports,
		INP int* n_config_used, INP FILE * fp, int* mux_max_size);
t_pb_graph_pin* get_pb_graph_node_pin_from_pb_graph_node(t_pb_graph_node *pb_graph_node, int ipin);
