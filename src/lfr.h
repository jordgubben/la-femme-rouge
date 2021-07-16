/****
La Femme Rough - An minimal graph based scripting system for games.
****/
#ifndef LFR_H
#define LFR_H

//// LFR Base types ////

typedef struct lfr_vec2_ { float x,y; } lfr_vec2_t;
#define LFR_VEC2_ORIGO ((lfr_vec2_t) { 0, 0})

typedef enum lfr_variant_type_ {
	lfr_nil_type,
	lfr_int_type,
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

lfr_variant_t lfr_int(int v) { return (lfr_variant_t) {lfr_int_type, .int_value = v}; }
lfr_variant_t lfr_float(float v) { return (lfr_variant_t) {lfr_float_type, .float_value = v}; }
lfr_variant_t lfr_vec2(lfr_vec2_t v) { return (lfr_variant_t) { lfr_vec2_type, .vec2_value = v}; }
lfr_variant_t lfr_vec2_xy(float x, float y) { return lfr_vec2((lfr_vec2_t){x,y}); }

float lfr_to_float(lfr_variant_t);

//// LFR Instructions ////

typedef enum lfr_instruction_ {
	lfr_print_own_id,
	lfr_randomize_number,
	lfr_add,
	lfr_sub,
	lfr_mul,
	lfr_distance,
	lfr_print_value,
	lfr_if_between,
	lfr_no_core_instructions // Not an instruction :P
} lfr_instruction_e;

bool lfr_is_core_instruction(unsigned bytecode) { return bytecode <= 0xff; }

// Forward declarations
struct lfr_vm_;
typedef struct lfr_vm_ lfr_vm_t;


//// LFR Node ////

typedef struct lfr_node_id_ { unsigned id; } lfr_node_id_t;

enum {lfr_signature_size = 8};
typedef struct lfr_node_ {
	unsigned instruction;
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
lfr_node_id_t lfr_insert_node_into_table(unsigned instruction, lfr_node_table_t*);
void lfr_change_node_id_in_table(lfr_node_id_t old_id, lfr_node_id_t new_id, lfr_node_table_t  *table);
unsigned lfr_get_node_index(lfr_node_id_t, const lfr_node_table_t *);
lfr_vec2_t lfr_get_node_position(lfr_node_id_t, const lfr_node_table_t *);
lfr_variant_t lfr_get_fixed_input_value(lfr_node_id_t, unsigned slot, const lfr_vm_t *, const lfr_node_table_t *);
lfr_variant_t lfr_get_default_output_value(lfr_node_id_t, unsigned, const lfr_vm_t *, const lfr_node_table_t *);
void lfr_set_node_position(lfr_node_id_t, lfr_vec2_t, lfr_node_table_t *);
void lfr_set_fixed_input_value(lfr_node_id_t, unsigned slot, lfr_variant_t, lfr_node_table_t *);
void lfr_set_default_output_value(lfr_node_id_t, unsigned slot, lfr_variant_t, lfr_node_table_t *);
void lfr_remove_node_from_table(lfr_node_id_t, lfr_node_table_t *);

// Node serialization
int lfr_save_nodes_in_table_to_file(const lfr_node_table_t*, const lfr_vm_t *, FILE * restrict stream);
int lfr_save_data_links_in_table_to_file(const lfr_node_table_t*, FILE * restrict stream);
int lfr_save_fixed_values_in_table_to_file(const lfr_node_table_t*, FILE * restrict stream);

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

// Node CRUD (for graph)
lfr_node_id_t lfr_add_node(lfr_instruction_e, lfr_graph_t *);
lfr_node_id_t lfr_add_custom_node(unsigned bytecode, lfr_graph_t *);
void lfr_remove_node(lfr_node_id_t, lfr_graph_t *);

// Flow link CRUD
void lfr_link_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);
bool lfr_has_link(lfr_node_id_t, lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_source_links(lfr_node_id_t, const lfr_graph_t*);
unsigned lfr_count_node_target_links(lfr_node_id_t, const lfr_graph_t*);
void lfr_unlink_nodes(lfr_node_id_t, lfr_node_id_t, lfr_graph_t*);
void lfr_disconnect_node(lfr_node_id_t, lfr_graph_t *);

// Data link CRUD
void lfr_link_data(lfr_node_id_t, unsigned, lfr_node_id_t, unsigned, lfr_graph_t*);
void lfr_unlink_input_data(lfr_node_id_t, unsigned, lfr_graph_t*);
void lfr_unlink_output_data(lfr_node_id_t, unsigned, lfr_graph_t*);

// Graph serialization
void lfr_load_graph_from_file_path(const char *, const lfr_vm_t *, lfr_graph_t *);
void lfr_load_graph_from_file(FILE * restrict stream, const lfr_vm_t *, lfr_graph_t *);
void lfr_save_graph_to_file_path(const lfr_graph_t *, const lfr_vm_t *, const char *path);
int lfr_save_graph_to_file(const lfr_graph_t *, const lfr_vm_t *, FILE * restrict stream);
int lfr_save_flow_links_to_file(const lfr_graph_t *, FILE * restrict stream);


//// LFR Instruction definitions ////

typedef enum lfr_result_ {
	lfr_halt,
	lfr_continue,
	lfr_no_results // Not a result :P
} lfr_result_e;

typedef struct lfr_instruction_def_ {
	const char *name;
	lfr_result_e (*func)(lfr_node_id_t, lfr_variant_t input[], lfr_variant_t output[], void*, const lfr_graph_t *);
	struct {
		const char* name;
		lfr_variant_t data;
	} input_signature[lfr_signature_size], output_signature[lfr_signature_size];
} lfr_instruction_def_t;

typedef struct lfr_vm_ {
	const lfr_instruction_def_t *custom_instructions;
	unsigned num_custom_instructions;
	void *custom_data;
} lfr_vm_t;

lfr_instruction_e lfr_find_instruction_from_name(const char* name, const lfr_vm_t *);
const char* lfr_get_instruction_name(unsigned, const lfr_vm_t *);
const char* lfr_get_core_instruction_name(lfr_instruction_e, const lfr_vm_t *);
const char* lfr_get_custom_instruction_name(unsigned, const lfr_vm_t *);
const struct lfr_instruction_def_* lfr_get_instruction(unsigned, const lfr_vm_t *);
const struct lfr_instruction_def_* lfr_get_core_instruction(lfr_instruction_e, const lfr_vm_t *);
const struct lfr_instruction_def_* lfr_get_custom_instruction(unsigned, const lfr_vm_t *);


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


lfr_variant_t lfr_get_input_value(lfr_node_id_t, unsigned slot,
	const lfr_vm_t *, const lfr_graph_t*, const lfr_graph_state_t*);
lfr_variant_t lfr_get_output_value(lfr_node_id_t, unsigned slot,
	const lfr_vm_t *, const lfr_graph_t*, const lfr_graph_state_t*);


//// LFR script execution ////

void lfr_schedule(lfr_node_id_t, const lfr_graph_t *, lfr_graph_state_t *);
int lfr_step(const lfr_vm_t *, const lfr_graph_t *, lfr_graph_state_t *);
lfr_result_e lfr_process_node_instruction(unsigned inst, lfr_node_id_t,
	const lfr_vm_t *, const lfr_graph_t *, lfr_graph_state_t *);

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
int lfr_step(const lfr_vm_t *vm, const lfr_graph_t *graph, lfr_graph_state_t *state) {
	if (!state->num_schedueled_nodes) { return 0; };

	// Find the right node
	const lfr_node_id_t node_id = state->schedueled_nodes[0];
	const unsigned node_index = T_INDEX(graph->nodes, node_id);
	const unsigned instruction = graph->nodes.node[node_index].instruction;

	// Process instruction
	lfr_result_e result = lfr_process_node_instruction(instruction, node_id, vm, graph, state);

	// Enqueue different nodes depending on processing result
	switch(result) {
	case lfr_continue: {
		// All clear - Continue flow throgh graph
		for (int i =0; i < graph->num_flow_links; i++) {
			const lfr_flow_link_t * link = &graph->flow_links[i];
			if (T_SAME_ID(link->source_node, node_id)) {
				lfr_schedule(link->target_node, graph, state);
			}
		}
	}break;
	case lfr_halt: {
		//Stop flow here - Do nothing
	} break;
	case lfr_no_results: { assert(0); } break;
	}

	// Shuffle queue (inefficient)
	for (int i = 1; i < state->num_schedueled_nodes; i++) {
		state->schedueled_nodes[i-1] = state->schedueled_nodes[i];
	}
	return state->num_schedueled_nodes--;
}


/**
Process a single node instruction.
**/
lfr_result_e lfr_process_node_instruction(unsigned instruction, lfr_node_id_t node_id,
		const lfr_vm_t *vm, const lfr_graph_t *graph, lfr_graph_state_t *state) {
	lfr_variant_t input[8] = {0}, output[8] = {0};

	// Get Input
	for (int i = 0; i < lfr_signature_size; i++) {
		input[i] = lfr_get_input_value(node_id, i, vm, graph, state);
	}

	// Process instruction
	const lfr_instruction_def_t *def = lfr_get_instruction(instruction, vm);
	lfr_result_e result = def->func(node_id, input, output, vm->custom_data, graph);

	// Update node state with new result data
	unsigned state_index = lfr_insert_node_state_at(node_id, &graph->nodes, &state->nodes);
	for (int i = 0; i < lfr_signature_size; i++) {
		state->nodes.node_state[state_index].output_data[i] = output[i];
	}

	return result;
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
Add a node with the given *core* instruction to the graph.

Node is positioned to the right of the last node in the table.
**/
lfr_node_id_t lfr_add_node(lfr_instruction_e inst, lfr_graph_t *graph) {
	assert(inst < lfr_no_core_instructions);
	lfr_node_id_t id = lfr_insert_node_into_table(inst, &graph->nodes);
	graph->nodes.position[graph->nodes.num_rows - 1] = graph->next_node_pos;
	graph->next_node_pos.x += 310;
	return id;
}


/**
Add a node with the given *custom* instruction to the graph.

Same layout system as core nodes.
**/
lfr_node_id_t lfr_add_custom_node(unsigned inst, lfr_graph_t *graph) {
	assert(graph);
	inst += 1 << 8;
	lfr_node_id_t id = lfr_insert_node_into_table(inst, &graph->nodes);
	graph->nodes.position[graph->nodes.num_rows - 1] = graph->next_node_pos;
	graph->next_node_pos.x += 310;
	return id;
}


/**
Remove node from graph, including all links to and from it.
**/
void lfr_remove_node(lfr_node_id_t id, lfr_graph_t *graph) {
	assert(graph && T_HAS_ID(graph->nodes, id));

	lfr_disconnect_node(id, graph);
	lfr_remove_node_from_table(id, &graph->nodes);
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
Completely disconnect given node from other nodes in the graph.
**/
void lfr_disconnect_node(lfr_node_id_t id, lfr_graph_t *graph) {
	assert(graph && T_HAS_ID(graph->nodes, id));

	// Disconnet from main flow
	for (int i = 0; i < graph->num_flow_links; i++) {
		lfr_flow_link_t *link = &graph->flow_links[i];
		if (T_SAME_ID(link->source_node, id) || T_SAME_ID(link->target_node, id)) {
			graph->flow_links[i--] = graph->flow_links[--graph->num_flow_links];
		}
	}

	// Disconnect data links
	for (int node_index = 0; node_index < graph->nodes.num_rows; node_index++) {
		lfr_node_t *node = &graph->nodes.node[node_index];

		// Clear all slots refering to the given node
		for (int slot = 0; slot < lfr_signature_size; slot++) {
			if (!T_SAME_ID(node->input_data[slot].node, id)) { continue; }
			node->input_data[slot].node = (lfr_node_id_t) { 0 };
		}
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
Load graph from the given file path.

Utility function that just handles file opening and closing for you.
**/
void lfr_load_graph_from_file_path(const char* file_path, const lfr_vm_t *vm, lfr_graph_t *graph) {
	FILE * fp = fopen(file_path, "r");
	lfr_load_graph_from_file(fp, vm, graph);
	fclose(fp);
}


/**
Load graph content from (tab separated) file.
**/
void lfr_load_graph_from_file(FILE * restrict stream, const lfr_vm_t *vm, lfr_graph_t *graph) {
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
			lfr_node_id_t id;
			char inst_buf[32];
			lfr_vec2_t pos;
			sscanf(line_buf, "node #%u %s (%f,%f)", &id.id, inst_buf, &pos.x, &pos.y);

			// Add instruction
			lfr_instruction_e instruction = lfr_find_instruction_from_name(inst_buf, vm);
			lfr_node_id_t tmp_id = lfr_insert_node_into_table(instruction, &graph->nodes);
			if (!T_SAME_ID(tmp_id, id)) {
				lfr_change_node_id_in_table(tmp_id, id, &graph->nodes);
			}
			lfr_set_node_position(id, pos, &graph->nodes);

		} else if (strcmp(type_buf, "data") == 0) {
			lfr_node_id_t output_node, input_node;
			unsigned output_slot, input_slot;
			sscanf(line_buf, "data #%u:%u -> #%u:%u",
				&output_node.id, &output_slot, &input_node.id, &input_slot);
			lfr_link_data(output_node, output_slot, input_node, input_slot, graph);
		} else if (strcmp(type_buf, "value") == 0) {
			lfr_node_id_t input_node;
			unsigned input_slot;
			char type_buf[9];
			int n;
			sscanf(line_buf, "value #%u:%u = %8s %n", &input_node.id, &input_slot, type_buf, &n);

			// Read type specific value from the rest of the line
			if (strcmp(type_buf, "float") == 0) {
				lfr_variant_t var = {lfr_float_type};
				sscanf(&line_buf[n], "%f", &var.float_value);
				lfr_set_fixed_input_value(input_node, input_slot, var, &graph->nodes);
			} else if (strcmp(type_buf, "int") == 0) {
				lfr_variant_t var = {lfr_int_type};
				sscanf(&line_buf[n], "%d", &var.int_value);
				lfr_set_fixed_input_value(input_node, input_slot, var, &graph->nodes);
			} else if (strcmp(type_buf, "vec2") == 0) {
				lfr_variant_t var = {lfr_vec2_type};
				sscanf(&line_buf[n], "(%f, %f)", &var.vec2_value.x, &var.vec2_value.y);
				lfr_set_fixed_input_value(input_node, input_slot, var, &graph->nodes);
			} else {
				fprintf(stderr,
					"%s():\tSkipping unknown type '%s' for #%u:%u.\n",
					__func__, type_buf, input_node.id, input_slot);
			}

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
Save graph to file at the given path.

Utility function that just handles file opening and closing for you.
**/
void lfr_save_graph_to_file_path(const lfr_graph_t *graph, const lfr_vm_t *vm, const char *path) {
	FILE * fp = fopen(path, "w");
	lfr_save_graph_to_file(graph, vm, fp);
	fclose(fp);
}


/**
Dump graph to file in a parsable (tab-separated) format.
**/
int lfr_save_graph_to_file(const lfr_graph_t *graph, const lfr_vm_t *vm, FILE * restrict stream) {
	int char_count = 0;

	// Dump things
	char_count += lfr_save_nodes_in_table_to_file(&graph->nodes, vm, stream);
	char_count += lfr_save_data_links_in_table_to_file(&graph->nodes, stream);
	char_count += lfr_save_fixed_values_in_table_to_file(&graph->nodes, stream);
	char_count += lfr_save_flow_links_to_file(graph, stream);

	return char_count;
}


/**
Dump main flow links in a parsable (tab-separated) format.
**/
int lfr_save_flow_links_to_file(const lfr_graph_t *graph, FILE * restrict stream) {
	int char_count = 0;

	// Flow links in graph
	for (int i = 0; i < graph->num_flow_links; i++) {
		const lfr_flow_link_t *link = &graph->flow_links[i];
		char_count += fprintf(stream, "link\t");
		char_count += fprintf(stream, "#%u -> #%u", link->source_node.id, link->target_node.id);
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

	// Insert row into sparse table with an unused id
	while(!table->next_id || T_HAS_ID(*table, (lfr_node_id_t) { table->next_id})) {
		table->next_id++;
		table->next_id %=lfr_node_table_id_range;
	};
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
Cnage the id of an existing table row to an unused if.

Note:
Although available in the pulblic API, this function moslty has internal usages.
It is used to change node IDs when loading files so that nodes get the ID definded in the file
and not just the next free one.
**/
void lfr_change_node_id_in_table(lfr_node_id_t old_id, lfr_node_id_t new_id, lfr_node_table_t  *table) {
	assert(T_HAS_ID(*table, old_id) && !T_HAS_ID(*table, new_id));
	unsigned index = T_INDEX(*table, old_id);
	table->dense_id[index] = new_id;
	table->sparse_id[new_id.id] = index;
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
lfr_variant_t lfr_get_fixed_input_value(
		lfr_node_id_t id, unsigned slot, const lfr_vm_t *vm, const lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);

	// Graph node fixed value
	lfr_variant_t graph_data = table->node[index].input_data[slot].fixed_value;
	if (graph_data.type != lfr_nil_type) {
		return graph_data;
	}

	// Instructions default value
	lfr_instruction_e inst = table->node[index].instruction;
	const lfr_instruction_def_t *inst_def = lfr_get_instruction(inst, vm);
	return inst_def->input_signature[slot].data;
}


/**
Get the default value for the given node and slot.
**/
lfr_variant_t lfr_get_default_output_value(
		lfr_node_id_t id, unsigned slot, const lfr_vm_t *vm, const lfr_node_table_t *table) {
	assert(slot < lfr_signature_size);

	unsigned index = T_INDEX(*table, id);

	// Graph node default
	lfr_variant_t graph_data = table->node[index].output_data[slot];
	if (graph_data.type != lfr_nil_type) {
		return graph_data;
	}

	// Instructions default value
	lfr_instruction_e inst = table->node[index].instruction;
	const lfr_instruction_def_t *inst_def = lfr_get_instruction(inst, vm);
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
Remove node table row.
**/
void lfr_remove_node_from_table(lfr_node_id_t  id, lfr_node_table_t *table) {
	assert(table && T_HAS_ID(*table, id));

	// Fast (unordered) delete by moviong last row
	unsigned index = T_INDEX(*table, id);
	unsigned moved = --table->num_rows;
	table->dense_id[index] =  table->dense_id[moved];
	table->node[index] =  table->node[moved];
	table->position[index] =  table->position[moved];

	// Finally update location of moved row
	table->sparse_id[table->dense_id[index].id] = index;
}


/**
Print nodes in table onto file stream in a parser friendly (tab separated) format.
**/
int lfr_save_nodes_in_table_to_file(const lfr_node_table_t *table, const lfr_vm_t *vm,FILE * restrict stream) {
	int char_count = 0;

	T_FOR_ROWS(index, *table) {
		char_count += fprintf(stream, "node\t");

		// ID
		lfr_node_id_t node_id = T_ID(*table,index);
		char_count += fprintf(stream, "#%u\t", node_id.id);

		// Instruction
		char_count += fprintf(stream, "%s\t", lfr_get_instruction_name(table->node[index].instruction, vm));

		// Position
		lfr_vec2_t pos = lfr_get_node_position(node_id, table);
		char_count += fprintf(stream, "(%f, %f)", pos.x, pos.y);

		char_count += fprintf(stream, "\n");
	}

	return char_count;
}


/**
Print data links between nodes onto file stream in a parser friendly (tab separated) format.
**/
int lfr_save_data_links_in_table_to_file(const lfr_node_table_t* table, FILE * restrict stream) {
	int char_count = 0;

	T_FOR_ROWS(index, *table) {
		lfr_node_id_t id = T_ID(*table, index);
		const lfr_node_t *node = &table->node[index];
		for (int slot = 0; slot < lfr_signature_size; slot++) {
			if (node->input_data[slot].node.id == 0) { continue; }

			char_count += fprintf(stream, "data\t");
			char_count += fprintf(stream,
				"#%u:%u -> #%u:%u",
				node->input_data[slot].node.id, node->input_data[slot].slot,
				id.id, slot);
			char_count += fprintf(stream, "\n");
		}
	}

	return char_count;
}


/**
Print fixed data values onto file stream in a parser friendly (tab separated) format.
**/
int lfr_save_fixed_values_in_table_to_file(const lfr_node_table_t* table, FILE * restrict stream) {
	int char_count = 0;

	T_FOR_ROWS(index, *table) {
		lfr_node_id_t id = T_ID(*table, index);
		const lfr_node_t *node = &table->node[index];
		for (int slot = 0; slot < lfr_signature_size; slot++) {
			if (node->input_data[slot].node.id != 0) { continue; }
			if (node->input_data[slot].fixed_value.type == lfr_nil_type) { continue; }

			// Print 'value' and slot
			char_count += fprintf(stream, "value\t");
			char_count += fprintf(stream, "#%u:%u =\t", id.id, slot);

			// Print type specific string to stream
			lfr_variant_t var = node->input_data[slot].fixed_value;
			if (var.type == lfr_float_type) {
				char_count += fprintf(stream, "float %f", var.float_value);
			} else if (var.type == lfr_int_type) {
				char_count += fprintf(stream, "int %d", var.int_value);
			} else if (var.type == lfr_vec2_type) {
				char_count += fprintf(stream,
					"vec2 (%f, %f)",
					var.vec2_value.x, var.vec2_value.y);
			} else {
				char_count += fprintf(stream, "???");
				fprintf(stderr,
					"%s():\tFailed to write unknown type for #%u:%u.\n",
					__func__, id.id, slot);
			}

			char_count += fprintf(stream, "\n");
		}
	}

	return char_count;
}


//// LFR Instructions ////

lfr_result_e lfr_print_own_id_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	unsigned node_index = T_INDEX(graph->nodes, node_id);
	printf("Node ID: [#%u|%u]\n", node_id.id, node_index);
	return lfr_continue;
}


lfr_result_e lfr_randomize_number_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	// Assign random float value
	output[0].type = lfr_float_type;
	output[0].float_value = (float)(rand())/(float)(RAND_MAX);
	return lfr_continue;
}


/**
Combine floats into a sum.
**/
lfr_result_e lfr_add_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
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
	return lfr_continue;
}


/**
Subtract - Get the difference between two floats.
**/
lfr_result_e lfr_sub_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	// Subtract the second float from the first
	if (input[0].type == lfr_float_type && input[1].type == lfr_float_type) {
		output[0] = lfr_float(input[0].float_value - input[1].float_value);
	} else {
		assert(0 && "Not two floats");
	}
	return lfr_continue;
}


/**
Multiply - Get the procuct of two (or more) floats
**/
lfr_result_e lfr_mul_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	// Multiply all floats
	lfr_variant_t result = {lfr_float_type, .float_value = 1};
	for (int i = 0; i < lfr_signature_size; i++) {
		if (input[i].type == lfr_float_type) {
			result.float_value *= input[i].float_value;
		}
	}

	// Assign result
	output[0] = result;
	return lfr_continue;
}


/**
Calculate the distance between two vec2.
**/
lfr_result_e lfr_distance_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	if (input[0].type == lfr_vec2_type && input[1].type == lfr_vec2_type) {
		lfr_vec2_t a = input[0].vec2_value;
		lfr_vec2_t b = input[1].vec2_value;
		float dx = a.x - b.x, dy = a.y - b.y;
		float l = sqrtf(dx * dx + dy * dy);
		output[0] = lfr_float(l);
	} else {
		fprintf(stderr,
			"Distance node [#%u|%u] recieved an unsuported input type combination.",
			node_id.id, lfr_get_node_index(node_id, &graph->nodes));
	}
	return lfr_continue;
}


/**
Print value to stdout.
**/
lfr_result_e lfr_print_value_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	switch(input[0].type) {
	case lfr_nil_type: { printf("nil\n");} break;
	case lfr_int_type: { printf("%d", input[0].int_value); } break;
	case lfr_float_type: { printf("%f\n", input[0].float_value); } break;
	case lfr_vec2_type: { printf("(%f,%f)\n", input[0].vec2_value.x, input[0].vec2_value.y); } break;
	case lfr_no_core_types: { assert(0 && "Not a type"); } break;
	}
	return lfr_continue;
}


/**
Only continue if value is within permitted range
**/
lfr_result_e lfr_if_between_proc(lfr_node_id_t node_id,
		lfr_variant_t input[], lfr_variant_t output[],
		void *custom_data,
		const lfr_graph_t* graph) {

	if (input[0].type == lfr_float_type) {
		float val = input[0].float_value;
		float min = lfr_to_float(input[1]);
		float max = lfr_to_float(input[2]);
		if (min <= val && val <= max) {
			return lfr_continue;
		}
	} else {
		fprintf(stderr,
			"If node [#%u|%u] recieved an unsuported input type in VAL.",
			node_id.id, lfr_get_node_index(node_id, &graph->nodes));
	}
	return lfr_halt;
}


/**
Look up table of all core instructions.

Note:
 Order is expected to match lfr_instruction_e above.
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
	{"sub", lfr_sub_proc,
		{{"A", {lfr_float_type, .float_value = 0}}, {"B", {lfr_float_type, .float_value = 0}}},
		{{"DIFF", {lfr_float_type, .float_value = 0}}}
	},
	{"mul", lfr_mul_proc,
		{{"A", {lfr_float_type, .float_value = 0}}, {"B", {lfr_float_type, .float_value = 0}}},
		{{"PROD", {lfr_float_type, .float_value = 0}}}
	},
	{"distance", lfr_distance_proc,
		{
			{"A", {lfr_vec2_type, .vec2_value = LFR_VEC2_ORIGO}},
			{"B", {lfr_vec2_type, .vec2_value = LFR_VEC2_ORIGO}}
		},
		{{"DIST", {lfr_float_type, .float_value = 0}}}
	},
	{"print_value", lfr_print_value_proc,
		{{"VAL", {lfr_float_type, .float_value = 0}}},
		{}
	},
	{"if_between", lfr_if_between_proc,
		{
			{"VAL", {lfr_float_type, .float_value = 0}},
			{"MIN", {lfr_float_type, .float_value = 0}},
			{"MAX", {lfr_float_type, .float_value = 0}}
		},
		{}
	},
};


/**
Get entire (core or custom) instruction definition.
**/
const lfr_instruction_def_t* lfr_get_instruction(lfr_instruction_e inst, const lfr_vm_t *vm) {
	if (inst < lfr_no_core_instructions) {
		return lfr_get_core_instruction(inst, vm);
	} else {
		inst -= 1<<8;
		return lfr_get_custom_instruction(inst, vm);
	}
}


/**
Get entire *core* instruction definition.
**/
const lfr_instruction_def_t* lfr_get_core_instruction(lfr_instruction_e inst, const lfr_vm_t *vm) {
	assert(inst < lfr_no_core_instructions);
	return &lfr_core_instructions_[inst];
}


/**
Get entire *custom* instruction definition.
**/
const lfr_instruction_def_t* lfr_get_custom_instruction(lfr_instruction_e inst, const lfr_vm_t *vm) {
	assert(inst < vm->num_custom_instructions);
	return &vm->custom_instructions[inst];
}


/**
Get name of (core or custom) instruction.
**/
const char* lfr_get_instruction_name(unsigned inst, const lfr_vm_t *vm) {
	return lfr_get_instruction(inst, vm)->name;
}


/**
Get name of *core* instruction.
**/
const char* lfr_get_core_instruction_name(lfr_instruction_e inst, const lfr_vm_t *vm) {
	return lfr_get_core_instruction(inst, vm)->name;
}


/**
Get name of *custom* instruction.
**/
const char* lfr_get_custom_instruction_name(unsigned inst, const lfr_vm_t *vm) {
	return lfr_get_custom_instruction(inst, vm)->name;
}


/**
Get (core or custom) instruction code from name.

Custom instructions are searched first to prevent new core instructions from breaking
existing scripts where a custom instruction has the same name.

**/
lfr_instruction_e lfr_find_instruction_from_name(const char* name, const lfr_vm_t *vm) {
	// Search among custom instructions
	for (int i = 0; i < vm->num_custom_instructions; i++) {
		if (strcmp(name, lfr_get_custom_instruction_name(i, vm)) == 0) {
			return i + (1 << 8);
		}
	}

	// Search among core intructions
	for (int i = 0; i < lfr_no_core_instructions; i++) {
		if (strcmp(name, lfr_get_core_instruction_name(i, vm)) == 0) {
			return i;
		}
	}

	// Warn, then fallback to intruction that does not do much at all
	const lfr_instruction_e fallback = lfr_print_own_id;
	printf("Unknown instruction '%s' substituted by '%s'\n", name, lfr_get_instruction_name(fallback, vm));
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
		const lfr_vm_t *vm, const lfr_graph_t *graph, const lfr_graph_state_t *state) {
	assert(graph && state);
	assert(T_HAS_ID(graph->nodes, id));
	assert(slot < lfr_signature_size);

	// Get data from linked output node slot if available
	unsigned index = T_INDEX(graph->nodes, id);
	const lfr_node_t *node = &graph->nodes.node[index];
	lfr_node_id_t out_node = node->input_data[slot].node;
	if (out_node.id) {
		unsigned out_slot = node->input_data[slot].slot;
		return lfr_get_output_value(out_node, out_slot, vm, graph, state);
	}

	// Otherwise return fixed value from (imutable) graph
	return lfr_get_fixed_input_value(id, slot, vm, &graph->nodes);
}


/**
Get current value for the given node and *output* slot.
**/
lfr_variant_t lfr_get_output_value(lfr_node_id_t id, unsigned slot,
		const lfr_vm_t *vm, const lfr_graph_t *graph, const lfr_graph_state_t *state) {
	assert(graph && state);
	assert(slot < lfr_signature_size);

	// Return state data if available
	if (lfr_node_state_table_contains(id, &state->nodes)) {
		unsigned index = T_INDEX(state->nodes, id);
		const lfr_node_state_t *node_state = &state->nodes.node_state[index];
		return node_state->output_data[slot];
	}

	// Otherwise return default
	return lfr_get_default_output_value(id, slot, vm, &graph->nodes);
}


//// Variant utils. ////

/**
Convert all variant types to a float.
**/
float lfr_to_float(lfr_variant_t var) {
	switch(var.type) {
	case lfr_nil_type: return 0;
	case lfr_int_type: return (float) var.int_value;
	case lfr_float_type: return var.float_value;
	case lfr_vec2_type: return var.vec2_value.x;
	case lfr_no_core_types: assert(0); return 0;
	}
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
