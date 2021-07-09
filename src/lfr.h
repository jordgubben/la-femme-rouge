/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

//// LFR Base types ////

typedef struct lfr_vec2_ { float x,y; } lfr_vec2_t;

typedef enum lfr_variant_type_ {
	lfr_nil_type,
	lfr_float_type,
	lfr_vec2_type,
	lfr_no_core_types
} lfr_variant_type_e;

typedef struct lfr_variant_ {
	lfr_variant_type_e type;
	union {
		int int_value;
		float float_value;
		lfr_vec2_t vec2_value;
	};
} lfr_variant_t;


//// LFR Instructions ////

typedef enum lfr_instruction_ {
	lfr_print_own_id,
	lfr_randomize_number,
	lfr_add,
	lfr_no_core_instructions // Not an instruction :P
} lfr_instruction_e;


//// LFR Node ////

typedef struct lfr_node_id_ { unsigned id; } lfr_node_id_t;

enum {lfr_signature_size = 8};
typedef struct lfr_node_ {
	lfr_instruction_e instruction;
	struct {
		lfr_node_id_t node;
		unsigned int slot;
		lfr_variant_t fixed_value;
	} input_data[lfr_signature_size];
	lfr_variant_t output_data[lfr_signature_size];
} lfr_node_t;

enum {lfr_node_table_max_rows = 16, lfr_node_table_id_range = 1024};
typedef struct lfr_node_table_ {
	// Meta fields
	unsigned sparse_id[lfr_node_table_id_range];
	lfr_node_id_t dense_id[lfr_node_table_max_rows];
	unsigned num_rows, next_id;

	// Data colums
	lfr_node_t node[lfr_node_table_max_rows];
	lfr_vec2_t position[lfr_node_table_max_rows];
} lfr_node_table_t;

// Node CRUD
lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e, lfr_node_table_t*);
unsigned lfr_get_node_index(lfr_node_id_t, const lfr_node_table_t *);
lfr_vec2_t lfr_get_node_position(lfr_node_id_t, const lfr_node_table_t *);
lfr_variant_t lfr_get_fixed_input_value(lfr_node_id_t, unsigned slot, const lfr_node_table_t *);
lfr_variant_t lfr_get_default_output_value(lfr_node_id_t, unsigned, const lfr_node_table_t *);
void lfr_set_node_position(lfr_node_id_t, lfr_vec2_t, lfr_node_table_t *);
void lfr_set_fixed_input_value(lfr_node_id_t, unsigned slot, lfr_variant_t, lfr_node_table_t *);
void lfr_set_default_output_value(lfr_node_id_t, unsigned slot, lfr_variant_t, lfr_node_table_t *);

// Node serialization
int lfr_save_node_table_to_file(const lfr_node_table_t*, FILE * restrict stream);


//// LFR Graph ////

typedef struct lfr_flow_link_ {
	lfr_node_id_t source_node, target_node;
} lfr_flow_link_t;

enum {lfr_graph_max_flow_links = 32};
typedef struct lfr_graph_ {
	// Nodes
	lfr_node_table_t nodes;
	lfr_vec2_t next_node_pos;

	// Flow links
	lfr_flow_link_t flow_links[lfr_graph_max_flow_links];
	unsigned num_flow_links;
} lfr_graph_t;

void lfr_init_graph(lfr_graph_t *);
void lfr_term_graph(lfr_graph_t *);
lfr_node_id_t lfr_add_node(lfr_instruction_e, lfr_graph_t *);

// Flow link CRUD
void lfr_link_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);
bool lfr_has_link(lfr_node_id_t, lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_source_links(lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_target_links(lfr_node_id_t, const lfr_graph_t*);
void lfr_unlink_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);

// Data link CRUD
void lfr_link_data(lfr_node_id_t, unsigned, lfr_node_id_t, unsigned, lfr_graph_t*);
void lfr_unlink_input_data(lfr_node_id_t, unsigned, lfr_graph_t*);
void lfr_unlink_output_data(lfr_node_id_t, unsigned, lfr_graph_t*);

// Graph serialization
void lfr_load_graph_from_file(FILE * restrict stream, lfr_graph_t *graph);
int lfr_save_graph_to_file(const lfr_graph_t *, FILE * restrict stream);
int lfr_save_links_to_file(const lfr_graph_t *, FILE * restrict stream);
// TODO: save data links


//// LFR Instruction definitions ////

typedef struct lfr_instruction_def_ {
	const char *name;
	void (*func)(lfr_node_id_t, lfr_variant_t input[], lfr_variant_t output[], const lfr_graph_t *);
	struct {
		const char* name;
		lfr_variant_t data;
	} input_signature[lfr_signature_size], output_signature[lfr_signature_size];
} lfr_instruction_def_t;

const char* lfr_get_instruction_name(lfr_instruction_e);
lfr_instruction_e lfr_find_instruction_from_name(const char* name);
const struct lfr_instruction_def_* lfr_get_instruction(lfr_instruction_e);


//// LFR Node state ////

typedef struct lfr_node_state_ {
	lfr_variant_t output_data[lfr_signature_size];
} lfr_node_state_t;

typedef struct lfr_node_state_table_ {
	// Meta fields (auxiliary table)
	unsigned sparse_id[lfr_node_table_id_range];
	lfr_node_id_t dense_id[lfr_node_table_max_rows];
	unsigned num_rows;

	// Data column(s)
	lfr_node_state_t node_state[lfr_node_table_max_rows];

} lfr_node_state_table_t;

// Node state CRUD
unsigned lfr_insert_node_state_at(lfr_node_id_t, const lfr_node_table_t*, lfr_node_state_table_t*);
bool lfr_node_state_table_contains(lfr_node_id_t, const lfr_node_state_table_t*);


//// LFR Graph state ////

enum { lfr_graph_state__max_queue = 8};
typedef struct lfr_graph_state_ {
	lfr_node_id_t schedueled_nodes[lfr_graph_state__max_queue];
	unsigned num_schedueled_nodes;

	lfr_node_state_table_t nodes;
} lfr_graph_state_t;


lfr_variant_t lfr_get_input_value(lfr_node_id_t, unsigned slot, const lfr_graph_t*, const lfr_graph_state_t*);
lfr_variant_t lfr_get_output_value(lfr_node_id_t, unsigned slot, const lfr_graph_t*, const lfr_graph_state_t*);


//// LFR script execution ////

void lfr_schedule(lfr_node_id_t, const lfr_graph_t *, lfr_graph_state_t *);
int lfr_step(const lfr_graph_t *, lfr_graph_state_t *);

#endif


/*	==============
	IMPLEMENTATION
	============== */
#ifdef LFR_IMPLEMENTATION
#undef LFR_IMPLEMENTATION

//// Sparce table macros ////
#define T_HAS_ID(t, r) \
	((t).sparse_id[(r).id] < (t).num_rows && (t).dense_id[(t).sparse_id[(r).id]].id == (r).id)

#define T_INDEX(t,r) \
	(assert(T_HAS_ID((t), (r))), (t).sparse_id[(r).id])

#define T_ID(table, index) \
	(assert( (index) < (table).num_rows), (table).dense_id[index])

/* Compare two ids. True if they are the same. */
#define T_SAME_ID(a,b) \
	((a).id == (b).id)

/* Add a new row to the sparse table, returning the index of the row.*/
#define T_INSERT_ROW(t, id_type) \
	((t).dense_id[(t).num_rows] = (id_type){(t).next_id}, (t).sparse_id[(t).next_id++] = (t).num_rows++)

#define T_FOR_ROWS(r,t) \
	for (unsigned r = 0; r < (t).num_rows; r++)


//// Utility macros ////
/* Log from a function, including the function name in the string. */
#define LFR_TRACE(m, ...) \
	printf("# %s():\t" m "\n", __func__, __VA_ARGS__);

//// Internals (defined further down) ////



//// LFR script execution ////

/**
Enqueue a node to process to the script executions todo-list (as long as there is space left in the queue).
**/
void lfr_schedule(lfr_node_id_t node_id, const lfr_graph_t *graph, lfr_graph_state_t *state) {
	assert(T_HAS_ID(graph->nodes, node_id));
	if (state->num_schedueled_nodes >= lfr_graph_state__max_queue) {
		fprintf(stderr, "%s(): Node queue is full!\n", __func__);
		return;
	}
	state->schedueled_nodes[state->num_schedueled_nodes++] = node_id;
}


/**
Execute topmost scheduled node (if any) from the script executions todo-list.

Returns number of scheduled nodes, including the one executed.
**/
int lfr_step(const lfr_graph_t *graph, lfr_graph_state_t *state) {
	if (!state->num_schedueled_nodes) { return 0; };

	// Find the right node
	const lfr_node_id_t node_id = state->schedueled_nodes[0];
	const unsigned node_index = T_INDEX(graph->nodes, node_id);
	const lfr_node_t *head = &graph->nodes.node[node_index];

	// Process instruction
	if (head->instruction < lfr_no_core_instructions) {
		lfr_variant_t input[8] = {0}, output[8] = {0};

		// Get Input
		for (int i = 0; i < lfr_signature_size; i++) {
			input[i] = lfr_get_input_value(node_id, i, graph, state);
		}

		// Process instruction
		lfr_get_instruction(head->instruction)->func(node_id, input, output, graph);

		// Update node state with new result data
		unsigned state_index = lfr_insert_node_state_at(node_id, &graph->nodes, &state->nodes);
		for (int i = 0; i < lfr_signature_size; i++) {
			state->nodes.node_state[state_index].output_data[i] = output[i];
		}
	}

	// Enqueue nodes - Continue flow throgh graph
	for (int i =0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t * link = &graph->flow_links[i];
		if (T_SAME_ID(link->source_node, node_id)) {
			lfr_schedule(link->target_node, graph, state);
		}
	}

	// Shuffle queue (inefficient)
	for (int i = 1; i < state->num_schedueled_nodes; i++) {
		state->schedueled_nodes[i-1] = state->schedueled_nodes[i];
	}
	return state->num_schedueled_nodes--;
}


//// LFR Graph ////

/**
Initialize an LFR graph.
**/
void lfr_init_graph(lfr_graph_t *graph) {
	graph->nodes.next_id = 1;
	graph->num_flow_links = 0;
	graph->next_node_pos = (lfr_vec2_t) {100,100};
}


/**
Terminate an LFR graph.
**/
void lfr_term_graph(lfr_graph_t *graph) {
}


/**
Add a node with the given instruction to the graph.

Node is positioned to the right of the last node in the table.
**/
lfr_node_id_t lfr_add_node(lfr_instruction_e inst, lfr_graph_t *graph) {
	lfr_node_id_t id = lfr_insert_node_into_table(inst, &graph->nodes);
	graph->nodes.position[graph->nodes.num_rows - 1] = graph->next_node_pos;
	graph->next_node_pos.x += 310;
	return id;
}


/**
Link execution of one node to another.
**/
void lfr_link_nodes(lfr_node_id_t source_node, lfr_node_id_t target_node, lfr_graph_t *graph) {
	assert(graph->num_flow_links < lfr_graph_max_flow_links);

	// Prevent duplicates
	if (lfr_has_link(source_node, target_node, graph)) { return; };

	// Add (non-duplicate link)
	graph->flow_links[graph->num_flow_links++] = (lfr_flow_link_t) {source_node, target_node};
}


/**
Is there a linke from one node to the other?
**/
bool lfr_has_link(lfr_node_id_t source_node, lfr_node_id_t target_node, const lfr_graph_t *graph) {
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		return true;
	}

	return false;
}


/**
How many links have this node as source?
**/
unsigned lfr_count_node_source_links(lfr_node_id_t source_node, const lfr_graph_t *graph) {
	unsigned count = 0;

	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( T_SAME_ID(source_node, link->source_node)) { count++; }
	}

	return count;
}


/**
How many links have this node as target?
**/
unsigned lfr_count_node_target_links(lfr_node_id_t target_node, const lfr_graph_t *graph) {
	unsigned count = 0;

	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		if ( T_SAME_ID(target_node, link->target_node)) { count++; }
	}

	return count;
}

/**
Break execution link from one node to another.
**/
void lfr_unlink_nodes(lfr_node_id_t source_node, lfr_node_id_t target_node, lfr_graph_t *graph) {
	for (int i = 0; i < graph->num_flow_links; i++) {
		lfr_flow_link_t *link = &graph->flow_links[i];
		if ( !T_SAME_ID(source_node, link->source_node)) { continue; }
		if ( !T_SAME_ID(target_node, link->target_node)) { continue; }

		// Remove (breaking order)
		graph->flow_links[i] = graph->flow_links[--graph->num_flow_links];
		return;
	}
}


/**
Link output data of one node with input data of another.

API note:
 Actually only uses the nodes table of the graph, but the entire graph is provided as input to
 keep API simmilar to that of linking node flow.
**/
void lfr_link_data(lfr_node_id_t out_node, unsigned out_slot, lfr_node_id_t in_node, unsigned in_slot,
		lfr_graph_t* graph) {
	assert(graph);
	assert(T_HAS_ID(graph->nodes, out_node) && T_HAS_ID(graph->nodes, in_node));
	assert(out_slot < lfr_signature_size && in_slot < lfr_signature_size);

	// Get node table (the only data we actuall need)
	lfr_node_table_t *table = &graph->nodes;

	// Set link
	unsigned in_index = T_INDEX(*table, in_node);
	table->node[in_index].input_data[in_slot].node = out_node;
	table->node[in_index].input_data[in_slot].slot = out_slot;
}


/**
Unlink given input node slot from any linked output slots.
**/
void lfr_unlink_input_data(lfr_node_id_t in_node, unsigned in_slot, lfr_graph_t *graph) {
	assert(graph && T_HAS_ID(graph->nodes, in_node));
	assert(in_slot < lfr_signature_size);

	// Clear node
	lfr_node_t *node = &graph->nodes.node[T_INDEX(graph->nodes, in_node)];
	node->input_data[in_slot].node = (lfr_node_id_t) {0};
	node->input_data[in_slot].slot = 0;
}


/**
Unlink given output node slot from all linked input slots.
**/
void lfr_unlink_output_data(lfr_node_id_t out_node, unsigned out_slot, lfr_graph_t *graph) {
	assert(graph && T_HAS_ID(graph->nodes, out_node));
	assert(out_slot < lfr_signature_size);

	// Cycle through all nodes
	for (int node_index = 0; node_index < graph->nodes.num_rows; node_index++) {
		lfr_node_t *node = &graph->nodes.node[node_index];

		// Cycle through all slots
		for (int in_slot = 0; in_slot < lfr_signature_size; in_slot++) {
			if (!T_SAME_ID(node->input_data[in_slot].node, out_node)) { continue; }
			if (node->input_data[in_slot].slot != out_slot) { continue; }

			// clear matching slot
			node->input_data[in_slot].node.id = 0;
			node->input_data[in_slot].slot = 0;
		}
	}
}


/**
Load graph content from (tab separated) file.
**/
void lfr_load_graph_from_file(FILE * restrict stream, lfr_graph_t *graph) {
	char line_buf[1024];
	while (fgets(line_buf, 1024, stream)) {
		// Get line type
		char type_buf[8];
		sscanf(line_buf, "%s", type_buf);

		// Parse various line types
		if (strlen(type_buf) == 0) {
			/* Skip if empty */
		} else if (strcmp(type_buf, "node") == 0) {
			// Parse instruction
			lfr_node_id_t expected_id;
			char inst_buf[32];
			lfr_vec2_t pos;
			sscanf(line_buf, "node #%u %s (%f,%f)", &expected_id.id, inst_buf, &pos.x, &pos.y);

			// Add instruction
			lfr_instruction_e instruction = lfr_find_instruction_from_name(inst_buf);
			lfr_node_id_t new_id = lfr_insert_node_into_table(instruction, &graph->nodes);
			assert(T_SAME_ID(new_id, expected_id));
			lfr_set_node_position(new_id, pos, &graph->nodes);

		} else if (strcmp(type_buf, "link") == 0) {
			// Parse and create link
			lfr_node_id_t source, target;
			sscanf(line_buf, "link #%u -> #%u", &source.id, &target.id);
			lfr_link_nodes(source, target, graph);

		} else {
			fprintf(stderr, "Unknown type '%s'\n", type_buf);
		}
	}
}

/**
Dump graph to file in a parsable (tab-separated) format.
**/
int lfr_save_graph_to_file(const lfr_graph_t *graph, FILE * restrict stream) {
	int char_count = 0;

	// Dump things
	char_count += lfr_save_node_table_to_file(&graph->nodes, stream);
	char_count += lfr_save_links_to_file(graph, stream);

	return char_count;
}


/**
Dump main flow links in a parsable (tab-separated) format.
**/
int lfr_save_links_to_file(const lfr_graph_t *graph, FILE * restrict stream) {
	int char_count = 0;

	// Flow links in graph
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		char_count += fprintf(stream, "link\t");
		char_count += fprintf(stream, "#%u -> #%u\t", link->source_node.id, link->target_node.id);
		char_count += fprintf(stream, "\n");
	}

	return char_count;
}


//// LFR Node table ////

/**
Insert a new node at the end of the table.
**/
lfr_node_id_t lfr_insert_node_into_table(lfr_instruction_e inst, lfr_node_table_t *table) {
	assert(table->num_rows < lfr_node_table_max_rows);

	// Insert row into sparse table
	int index = T_INSERT_ROW(*table, lfr_node_id_t);

	// Set row data
	table->node[index].instruction = inst;
	for (int i = 0; i < lfr_signature_size; i++) {
		table->node[index].output_data[i] = (lfr_variant_t) { lfr_nil_type, 0};
	}
	table->position[index] = (lfr_vec2_t) { 0, 0};

	return table->dense_id[index];
}


/**
Get node index for the given id.
**/
unsigned lfr_get_node_index(lfr_node_id_t id, const lfr_node_table_t *table) {
	return T_INDEX(*table, id);
}

/**
Get position of a node in the table.
**/
lfr_vec2_t lfr_get_node_position(lfr_node_id_t id, const lfr_node_table_t *table) {
	return table->position[T_INDEX(*table, id)];
}


/**
Get (fixed) input value for given node and slot.
**/
lfr_variant_t lfr_get_fixed_input_value(lfr_node_id_t id, unsigned slot, const lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);

	// Graph node fixed value
	lfr_variant_t graph_data = table->node[index].input_data[slot].fixed_value;
	if (graph_data.type != lfr_nil_type) {
		return graph_data;
	}

	// Instructions default value
	lfr_instruction_e inst = table->node[index].instruction;
	const lfr_instruction_def_t *inst_def = lfr_get_instruction(inst);
	return inst_def->input_signature[slot].data;
}


/**
Get the default value for the given node and slot.
**/
lfr_variant_t lfr_get_default_output_value(lfr_node_id_t id, unsigned slot, const lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);

	// Graph node default
	lfr_variant_t graph_data = table->node[index].output_data[slot];
	if (graph_data.type != lfr_nil_type) {
		return graph_data;
	}

	// Instructions default value
	lfr_instruction_e inst = table->node[index].instruction;
	const lfr_instruction_def_t *inst_def = lfr_get_instruction(inst);
	return inst_def->output_signature[slot].data;
}


/**
Set position of a node in the table.
**/
void lfr_set_node_position(lfr_node_id_t id, lfr_vec2_t pos, lfr_node_table_t *table) {
	table->position[T_INDEX(*table, id)] = pos;
}


/**
Set a fixed value as input for the given node and slot.

Note:
 Breaks/Clears any previously existing data link.
**/
void lfr_set_fixed_input_value(lfr_node_id_t id, unsigned slot, lfr_variant_t value, lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);
	table->node[index].input_data[slot].node = (lfr_node_id_t) {0};
	table->node[index].input_data[slot].fixed_value = value;
}


/**
Set default date for the given node and slot.
**/
void lfr_set_default_output_value(lfr_node_id_t id, unsigned slot, lfr_variant_t value, lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);
	table->node[index].output_data[slot] = value;
}


/**
Print node table content onto file stream in a parser friendly (tab separated) format.
**/
int lfr_save_node_table_to_file(const lfr_node_table_t *table, FILE * restrict stream) {
	int char_count = 0;

	T_FOR_ROWS(index, *table) {
		char_count += fprintf(stream, "node\t");

		// ID
		lfr_node_id_t node_id = T_ID(*table,index);
		char_count += fprintf(stream, "#%u\t", node_id.id);

		// Instruction
		char_count += fprintf(stream, "%s\t", lfr_get_instruction_name(table->node[index].instruction));

		// Position
		lfr_vec2_t pos = lfr_get_node_position(node_id, table);
		char_count += fprintf(stream, "(%f, %f)\t", pos.x, pos.y);

		char_count += fprintf(stream, "\n");
	}

	return char_count;
}


//// LFR Instructions ////

void lfr_print_own_id_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		const lfr_graph_t* graph) {

	unsigned node_index = T_INDEX(graph->nodes, node_id);
	printf("Node ID: [#%u|%u]\n", node_id.id, node_index);
}


void lfr_randomize_number_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		const lfr_graph_t* graph) {

	// Assign random float value
	output[0].type = lfr_float_type;
	output[0].float_value = (float)(rand())/(float)(RAND_MAX);
	return;
}


/**
Combine floats into a sum.
**/
void lfr_add_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		const lfr_graph_t* graph) {

	// Sum all floats
	lfr_variant_t result = {lfr_float_type, .float_value = 0};
	for (int i = 0; i < lfr_signature_size; i++) {
		if (input[i].type == lfr_float_type) {
			result.float_value += input[i].float_value;
		}
	}

	// Assign result
	output[0] = result;
}

/**
Look up table of all core instructions.
**/
static const lfr_instruction_def_t lfr_core_instructions_[lfr_no_core_instructions] = {
	{"print_own_id", lfr_print_own_id_proc, {}, {}},
	{"randomize_number", lfr_randomize_number_proc,
		{},
		{{"RND float",  {lfr_float_type, .float_value = 0 }}}
	},
	{"add", lfr_add_proc,
		{{"A", {lfr_float_type, .float_value = 0}}, {"B", {lfr_float_type, .float_value = 0}}},
		{{"SUM", {lfr_float_type, .float_value = 0}}}
	},
};


/**
Get entire (code) instruction definition.
**/
const lfr_instruction_def_t* lfr_get_instruction(lfr_instruction_e inst) {
	assert(inst < lfr_no_core_instructions);
	return &lfr_core_instructions_[inst];
}


/**
Get name of (core) instruction.
**/
const char* lfr_get_instruction_name(lfr_instruction_e inst) {
	return lfr_get_instruction(inst)->name;
}


/**
Get (core) instruction code from name.
**/
lfr_instruction_e lfr_find_instruction_from_name(const char* name) {
	for (int i = 0; i < lfr_no_core_instructions; i++) {
		if (strcmp(name, lfr_get_instruction_name(i)) == 0) {
			return i;
		}
	}

	// Warn, then fallback to intruction that does not do much at all
	const lfr_instruction_e fallback = lfr_print_own_id;
	printf("Unknown instruction '%s' substituted by '%s'\n", name, lfr_get_instruction_name(fallback));
	return fallback;
}


//// LFR Node state ////


/**
Insert a row for the given id into the auxiliary node state table.
**/
unsigned lfr_insert_node_state_at(lfr_node_id_t id, const lfr_node_table_t * nt, lfr_node_state_table_t *st) {
	assert(nt && st);
	assert(T_HAS_ID(*nt, id));

	// Reuse existing row or create new
	unsigned index;
	if (T_HAS_ID(*st, id)) {
		index = T_INDEX(*st, id);
	} else {
		assert(st->num_rows < lfr_node_table_max_rows);
		index = st->num_rows++;
		st->dense_id[index] = id;
		st->sparse_id[id.id] = index;
	}

	return index;
}

/**
Is the given id correspond to a row in the given node state table?
**/
bool lfr_node_state_table_contains(lfr_node_id_t id, const lfr_node_state_table_t *table) {
	return T_HAS_ID(*table, id);
}


//// LFR Graph state ////


/**
Get current value for the given node and *input* slot.
**/
lfr_variant_t lfr_get_input_value(lfr_node_id_t id, unsigned slot,
		const lfr_graph_t *graph, const lfr_graph_state_t *state) {
	assert(graph && state);
	assert(T_HAS_ID(graph->nodes, id));
	assert(slot < lfr_signature_size);

	// Get data from linked output node slot if available
	unsigned index = T_INDEX(graph->nodes, id);
	const lfr_node_t *node = &graph->nodes.node[index];
	lfr_node_id_t out_node = node->input_data[slot].node;
	if (out_node.id) {
		unsigned out_slot = node->input_data[slot].slot;
		return lfr_get_output_value(out_node, out_slot, graph, state);
	}

	// Otherwise return fixed value from (imutable) graph
	return lfr_get_fixed_input_value(id, slot, &graph->nodes);
}


/**
Get current value for the given node and *output* slot.
**/
lfr_variant_t lfr_get_output_value(lfr_node_id_t id, unsigned slot,
		const lfr_graph_t *graph, const lfr_graph_state_t *state) {
	assert(graph && state);
	assert(slot < lfr_signature_size);

	// Return state data if available
	if (lfr_node_state_table_contains(id, &state->nodes)) {
		unsigned index = T_INDEX(state->nodes, id);
		const lfr_node_state_t *node_state = &state->nodes.node_state[index];
		return node_state->output_data[slot];
	}

	// Otherwise return default
	return lfr_get_default_output_value(id, slot, &graph->nodes);
}

#undef T_HAS_ID
#undef T_INDEX
#undef T_ID
#undef T_FOR_ROWS
#undef LFR_TRACE
#endif


/********************************************************************************
MIT License
===========

Copyright (c) 2021 Jakob Eklund

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************************/
