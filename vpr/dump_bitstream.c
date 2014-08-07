#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "pb_type_graph.h"
#include "rr_graph.h"
#include "rr_graph_util.h"
#include "verilog_writer.h"
#include "dump_bitstream.h"

void config_set(int high, int low, int value, char** config_array, int* config_array_size, FILE * fp) {
	
	int curr_iarray = *config_array_size/8;
	int curr_bit_used = *config_array_size%8;
	int bits_req = high-low+1;
	int i;
	char c;
		
	//first time configuring, use malloc
	if (*config_array_size == 0) {	
		*config_array_size += high-low+1;
		*config_array = (char*)malloc((*config_array_size/8)*sizeof(char));
	} 
	//case where you have bits unused and require less than that
	//if curr_bit_used != 0, there are some unused bits
	//if (8bits - curr_bit_used) > bits_req), there are enough unused bits
	else if (curr_bit_used!=0 && (8-curr_bit_used) > bits_req) {
		*config_array_size += high-low+1;
	} else { //if not enough
		*config_array_size += high-low+1;
		*config_array = (char*)realloc(*config_array, (int)ceil((double)(*config_array_size)/8)*sizeof(char));
	}
	
	//for each bit, set the values
	for (i=0; i<bits_req; i++) {	
			
		//if there are space, keep filling it up from [0:7]
		if (curr_bit_used<8) curr_bit_used++;
		else { //all filled up, set count back to 0 and go to the next array
			curr_bit_used = 0;
			curr_iarray++;
		}
		c = (*config_array)[curr_iarray];
		c = (c << 1) & 254;
		c += value>>(i)&1;
		(*config_array)[curr_iarray] = c;
	}
	assert(curr_iarray < ((int)ceil((double)(*config_array_size)/8)));

}

void config_set_block (int high, int low, \
	int* logical_blk_indices, int logical_blk_cap, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp) {
	
	int icap, ipin, iblk;
	int config_incr, config_curr;
	int config_val;
	
	int* config_used = (int*)calloc(1, sizeof(int));
		
	config_incr = (high - low + 1)/logical_blk_cap;
	config_curr = low;
	
	t_pb_graph_node *pb_graph_node;
	for (icap=0; icap<logical_blk_cap; icap++) {
		iblk = logical_blk_indices[icap];
		if (iblk != -1) {
			fprintf(fp, "[%d:%d] \"%s\" %s %d %d", config_curr+config_incr-1, config_curr, block[iblk].name, \
				block[iblk].pb->pb_graph_node->pb_type->name, block[iblk].x, block[iblk].y);
			pb_graph_node = block[iblk].pb->pb_graph_node;
			if (strcmp(pb_graph_node->pb_type->name, "io") == 0) {
				for (ipin=0; ipin<pb_graph_node->total_pb_pins ; ipin++) {
					if (block[iblk].pb->rr_graph[ipin].prev_node != -1) {
						if (block[iblk].pb->rr_graph[ipin].pb_graph_pin->port->type == OUT_PORT) {
							fprintf(fp, " in %d ", icap);
							config_val = 1;
						} else {
							fprintf(fp, " out %d ", icap);
							config_val = 0;
						}
						fprintf(fp, "%d\n", config_val);
						config_set(config_curr+config_incr-1, config_curr, config_val, config_array, config_array_size, fp);
						config_curr += config_incr;
					}
				}
			} else {
				fprintf(fp, "\n");
				*config_used = config_curr;
				config_set_block_rec(block[iblk].pb, config_used, net_order, \
					config_array, config_array_size, fp);
				for (int i=0; i<pb_graph_node->total_pb_pins; i++) {
					if (strcmp(pb_graph_node->pb_type->name, \
							block[iblk].pb->rr_graph[i].pb_graph_pin->parent_node->pb_type->name)==0) {
						fprintf(fp, "\t\t%s[%d].%s[%d]   \t -> \t [%d] -> ", \
							block[iblk].pb->rr_graph[i].pb_graph_pin->parent_node->pb_type->name,\
							block[iblk].pb->rr_graph[i].pb_graph_pin->parent_node->placement_index,\
							block[iblk].pb->rr_graph[i].pb_graph_pin->port->name, \
							block[iblk].pb->rr_graph[i].pb_graph_pin->pin_number, \
							block[iblk].pb->rr_graph[i].net_num);
						if (block[iblk].pb->rr_graph[i].pb_graph_pin->port->type == OUT_PORT) {
							fprintf(fp, "[%d]\n", net_order[2][block[iblk].pb->rr_graph[i].pb_graph_pin->port->port_index_by_type][block[iblk].pb->rr_graph[i].pb_graph_pin->pin_number][0]);	
						} else {	
							fprintf(fp, "[%d]\n", net_order[1][block[iblk].pb->rr_graph[i].pb_graph_pin->port->port_index_by_type][block[iblk].pb->rr_graph[i].pb_graph_pin->pin_number][0]);	
						}
					}
				}
			}
		} else {
			fprintf(fp, "[%d:%d] empty\n", config_curr+config_incr-1,  config_curr);
			config_val = 0;
			for (int i=0; i<config_incr; i++) {
				config_set(config_curr+i, config_curr+i, config_val, config_array, config_array_size, fp);
			}
			config_curr = config_curr+config_incr;
		}
	}
	fprintf(fp, "\n");
}

void config_set_empty_block_rec (t_pb_graph_node *pb_graph_node, int* low_config, 
	char** config_array, int* config_array_size, FILE * fp) {
	
	int i, j, k;	
	if (strcmp(pb_graph_node->pb_type->name, "lut6") == 0) {
		//config_set_LUT(pb, low_config, \
			config_array, config_array_size, fp);
			
		fprintf(fp, "\t[%d:%d] (LUT) unused\n",64+*low_config-1, *low_config);
		for (i=0; i<64; i++) {
			config_set(*low_config, *low_config, 0, config_array, config_array_size, fp);
			(*low_config)++;
		}
		return;
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k<pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				config_set_empty_pins(pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins, \
					pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports, \
					low_config, config_array, config_array_size, fp);
				config_set_empty_block_rec(&pb_graph_node->child_pb_graph_nodes[i][j][k], low_config, 
					config_array, config_array_size, fp);
			}
		}
	}
	config_set_empty_pins(pb_graph_node->output_pins, \
		pb_graph_node->num_output_ports, low_config, \
		config_array, config_array_size, fp);
}

void config_set_block_rec (t_pb *pb, int* low_config, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp) {
	
	int i, j, k;	
	t_pb_graph_node *pb_graph_node = pb->pb_graph_node;
								
	if (strcmp(pb_graph_node->pb_type->name, "lut6") == 0) {
		config_set_LUT(pb, low_config, \
			config_array, config_array_size, fp);
		return;
	}
	
	if (pb->name == NULL) {
		config_set_empty_block_rec (pb_graph_node, low_config, config_array, config_array_size, fp);
		return;
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0;	k<pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;	k++) {
				if (pb->child_pbs[j][k].name == NULL) {
					config_set_empty_pins(pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins, \
						pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports, \
						low_config, config_array, config_array_size, fp);
				} else {
					config_set_pins(pb_graph_node->child_pb_graph_nodes[i][j][k].input_pins, \
						pb_graph_node->child_pb_graph_nodes[i][j][k].num_input_ports, \
						low_config, pb->rr_graph, net_order, config_array, config_array_size, fp);
				}
				config_set_block_rec(&pb->child_pbs[j][k], low_config, net_order, 
					config_array, config_array_size, fp);
			}
		}
	}
	config_set_pins(pb_graph_node->output_pins, \
		pb_graph_node->num_output_ports, low_config, \
		pb->rr_graph, net_order, config_array, config_array_size, fp);

}

void config_set_empty_pins(t_pb_graph_pin **pb_graph_pins, int num_ports, \
	int* low_config, char** config_array, int* config_array_size, FILE * fp) {
		
	int i, j;
	int fan_in, mux_size;
	int config_val = 0;
	
	t_port *port;
	for (i = 0; i < num_ports; i++) {
		port = pb_graph_pins[i][0].port;
		for (j = 0; j < port->num_pins; j++) {
			assert(j == pb_graph_pins[i][j].pin_number);
			assert(pb_graph_pins[i][j].port == port);
			fan_in = pb_graph_pins[i][j].num_input_edges;
			
			//similar to architecture pin connection
			//if fan_in > 1, mux needs to be instantiated
			if (fan_in>1) {			
				mux_size = (int)ceil(log(fan_in)/log(2));
				config_val = 0;
				fprintf(fp, "\t[%d:%d] (%s[%d].%s[%d]) unused\n", mux_size +*low_config-1, *low_config, \
					pb_graph_pins[i][j].parent_node->pb_type->name, pb_graph_pins[i][j].parent_node->placement_index, \
					pb_graph_pins[i][j].port->name, pb_graph_pins[i][j].pin_number);
				config_set(mux_size+*low_config-1, *low_config, \
					config_val, config_array, config_array_size, fp);
				*low_config+=mux_size;
			}
		} 
	}
}

void config_set_pins(t_pb_graph_pin **pb_graph_pins, int num_ports, \
	int* low_config, s_rr_node *rr_graph_of_cluster, int**** net_order, \
	char** config_array, int* config_array_size, FILE * fp) {
		
	int i, j, k, m;
	int fan_in, mux_size;
	int config_val, old_config_val;
	
	int pin_count_in_cluster;
	int edge_pin_count_in_cluster;
	int net_num, new_net_num;
	t_port *port, *edge_port;
	for (i = 0; i < num_ports; i++) {
		port = pb_graph_pins[i][0].port;
		for (j = 0; j < port->num_pins; j++) {
			assert(j == pb_graph_pins[i][j].pin_number);
			assert(pb_graph_pins[i][j].port == port);
			fan_in = pb_graph_pins[i][j].num_input_edges;
						
			//similar to architecture pin connection
			//if fan_in > 1, mux needs to be instantiated
			if (fan_in>1) {	
				if (rr_graph_of_cluster[pb_graph_pins[i][j].pin_count_in_cluster].net_num == OPEN) {
					old_config_val = 0;
					config_val = 0;				
				} else {
					pin_count_in_cluster = pb_graph_pins[i][j].pin_count_in_cluster;					
					net_num = rr_graph_of_cluster[pin_count_in_cluster].net_num;
					for (k = 0; k < fan_in; k++) {
						for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {
							edge_pin_count_in_cluster = pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_count_in_cluster;
							edge_port = pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port;
							if (edge_port->type == OUT_PORT)
								new_net_num = rr_graph_of_cluster[edge_pin_count_in_cluster].net_num;	
							else
								new_net_num = net_order[1][edge_port->port_index_by_type][pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number][0];
							//fprintf(fp, "net_num = %d, new_net_num = %d\n", net_num, new_net_num);
							
							if (net_num == new_net_num) {
								if (port->type == OUT_PORT) config_val = fan_in-k-1;
								else config_val = fan_in-k; //NO -1 for inputs only since there's a GND signal
							} 
						}
					}
					for (k = 0; k < fan_in; k++) {
						for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {						
							pin_count_in_cluster = pb_graph_pins[i][j].pin_count_in_cluster;
							edge_pin_count_in_cluster = pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_count_in_cluster;
							if (rr_graph_of_cluster[pin_count_in_cluster].prev_node == edge_pin_count_in_cluster) {

								net_num = rr_graph_of_cluster[pin_count_in_cluster].net_num;
								if (port->type == OUT_PORT) old_config_val = fan_in-k-1;
								else old_config_val = fan_in-k; //NO -1 for inputs only since there's a GND signal

							}  
						}
					}
					
				}
				
				mux_size = (int)ceil(log(fan_in)/log(2));
				fprintf(fp, "\t[%d:%d] (%s[%d].%s[%d]) [%d] -> [%d]\n",mux_size +*low_config-1, *low_config, \
					pb_graph_pins[i][j].parent_node->pb_type->name, pb_graph_pins[i][j].parent_node->placement_index, \
					pb_graph_pins[i][j].port->name, pb_graph_pins[i][j].pin_number, old_config_val, config_val);
				config_set(mux_size+*low_config-1, *low_config, \
					config_val, config_array, config_array_size, fp);
				*low_config+=mux_size;
			}
		} 
	}

}

void config_set_LUT(t_pb *pb, int* low_config, \
	char** config_array, int* config_array_size, FILE * fp) {
	
	int a_net, p_net, p_pin_count;
	int i, j, k;
	
	int *lut_pin_remap;
	int num_input;
	
	s_linked_vptr *row;
	char *output, *original, *copy;

	s_linked_vptr* head_truth_table = (s_linked_vptr*)malloc(sizeof(s_linked_vptr));

	num_input = find_number_of_inputs(pb);

	if (pb->mode == 1) {
		if (pb->child_pbs == NULL) {
			/*output = (char*)malloc((int)pow(2, num_input)*sizeof(char));
			for (i=0; i<(int)pow(2, num_input); i++) {
				output[i] = '0';
			}*/
			head_truth_table = NULL;
		} else {
		

			pb = &pb->child_pbs[0][0];
			//assert num_input_ports!!!!
			//num_input = pb->pb_graph_node->num_input_pins[0];
			lut_pin_remap = (int*)malloc(num_input*sizeof(int));
			for (j=0; j<num_input; j++) {
				p_pin_count = pb->pb_graph_node->input_pins[0][j].pin_count_in_cluster;
				p_net = pb->parent_pb->rr_graph[p_pin_count].net_num;			
				if (p_net == OPEN) {
					lut_pin_remap[j] = OPEN;
					continue;
				}
				for (k=0; k<num_input; k++) {
					a_net = logical_block[pb->logical_block].input_nets[0][k];
					if (a_net == p_net) {
						lut_pin_remap[j] = k;
					}
				}
			}
	
			for (i=0; i<num_input; i++) {
				fprintf(fp, "\t\t\t%d -> %d\n", i, lut_pin_remap[i]);
			}

			s_linked_vptr* curr_row = head_truth_table;
		
			for(row=logical_block[pb->logical_block].truth_table; row!=NULL; row=row->next){
		
				original = (char*)row->data_vptr;		
				curr_row->data_vptr = (char*)malloc((strlen(original)+1)*sizeof(char));
				copy = (char*)curr_row->data_vptr;
				strcpy(copy, original);
				copy[strlen(original)] = '\0';
				for (i=0; i<num_input; i++) {
					if (lut_pin_remap[i] != OPEN) {
						copy[i] = original[lut_pin_remap[i]];
					}	
				}
				if (row->next == NULL) curr_row->next = NULL;
				else {
					curr_row->next = (s_linked_vptr*)malloc(sizeof(s_linked_vptr));
					curr_row = curr_row->next;
				}
			}
		}
	} else {
		head_truth_table->data_vptr = (char*)malloc((4)*sizeof(char));
		copy = (char*)head_truth_table->data_vptr;
		copy = "1 1\0";

		head_truth_table->next = NULL;
	}
	
	output = load_truth_table_without_pb(num_input, head_truth_table);
	free (head_truth_table);

	fprintf(fp, "\t[%d:%d] (LUT %s) \n", 64+*low_config-1, *low_config, pb->name);

	for (i=(int)pow(2, num_input)-1; i>=0; i--) {
		config_set(*low_config, *low_config, output[i]-'0', config_array, config_array_size, fp);
		(*low_config)++;
		fprintf(fp, "%c", output[i]);
	}
	fprintf(fp, "\n");
}

char *load_truth_table_without_pb(int inputs , s_linked_vptr *truth_table)
{
  int number_of_dont_cares=0;
  int tries,shift,which_row,i,j;
  int possibles = 1 << inputs;
  char *tt_row_blif;
  char *possible_row = (char *)malloc(inputs+1 * sizeof(char));
  char *tt = (char *)malloc((possibles+1) * sizeof(char));
  struct s_linked_vptr *current;
  int number_of_used_inputs_to_lut;
  char set_to;
  assert(possible_row);
  assert(tt);
 
  if (truth_table) { // ??? janders ???? how can you have a LUT without a truth table???
    tt_row_blif =  (char *)truth_table->data_vptr;
  }
  else { // must be producing a GROUND
    for (i = 0; i < possibles; i++)
      tt[i] = '0';
    tt[possibles] = '\0';
    free(possible_row);
    return tt;
  }
  set_to = tt_row_blif[strlen(tt_row_blif)-1];

  /*filling the truth table with the state that is opposite to the output state in the blif truth table*/
  if(set_to =='1')
    {
      for(i=0 ; i<possibles ; i++)
        {
          tt[i]='0';
        }
    }
  else{
    for(i=0 ; i<possibles ; i++)
      {
        tt[i]='1';
      }
  }
  tt[possibles] = '\0';/*null terminating the tt string*/
  /*The code bellow will populate the tt[] array with the truth table of the LUT with "pb"
    
    The truth table is stored as a linked list in the VPR. Each node in the linked list stores one line of the truth table. This line is in the exact same format as in the blif file. Each line of the truth table is read by traversing through each node of the linked list. The code bellow then parses each line of the truth table. That line might not have all the inputs to the lut specified, or there could be don't cares in that line of the truth table. at the end we need the output value of the LUT for every possible combination of inputs, which is not given in the blif truth table.

  The code bellow first counts the number of don't cares in that line of the truth table. It also creates a temporary string "possible_row". and there are n=2^(number of don't cares) possibilities in a particular line of the truth table So in a separate loop we find the nth possibility of the truth table*/

  for(current=truth_table ; current!=NULL ; current=current->next)/*traversing through all the lines of the blif truth table*/
    {

      tt_row_blif=(char *)current->data_vptr;/*tt_row_blif stores the current truth table line*/

      /*counting number of don't cares in this line of the blif truth table*/
      number_of_used_inputs_to_lut = strlen(tt_row_blif)-2;/*used*/
      if(number_of_used_inputs_to_lut == 0)/*If this is a constant generator, then the truth table only contains the constant value that it generates.*/
        {
          for(i=0 ; i<possibles ; i++)
            {
              tt[i] = tt_row_blif[1];
            }
        }
      else{
        for(i=0; i<inputs ; i++)
          {
            if(tt_row_blif[i] == '-')
              {
                number_of_dont_cares++;
              }
          }
        tries = 1 << number_of_dont_cares;/*how many possibilities are there for this line of the blif truth table*/

        for(i=0 ; i<(tries) ; i++)/*traverse through all the possibilities for this line of the blif truth table*/
          {
            shift=number_of_dont_cares-1;
            for(j=0 ; j<number_of_used_inputs_to_lut ; j++)/*going through all the columns of the blif truth table*/
              {
                if(tt_row_blif[j]=='-')/*if this column represents a don't care*/
                  {
                    /*creating a temporary truth table row by taking different values of the possiblity 'i' depending on the shift value*/
                    if(((i>>shift) & 0x1) == 1)
                      {
                        possible_row[j] = '1';
                      }
                    else{
                      possible_row[j] = '0';
                    }
                    shift--;/*reduce the shift value by one*/
                  }
                else
                  {
                    possible_row[j]=tt_row_blif[j];
                  }
              }
            possible_row[j]=0x0;/*null terminate the temporary truth table row*/
            which_row = find_index(possible_row,inputs);/*find index returns the index of the 64bit truth table that this temporary truth table row corresponds to*/
            tt[possibles-1-which_row] = set_to;
          }
      }
      number_of_dont_cares = 0;
    }
  free(possible_row);
  return(tt);
}



